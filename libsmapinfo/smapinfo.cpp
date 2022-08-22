/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <inttypes.h>
#include <linux/oom.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include <iomanip>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include <android-base/file.h>
#include <android-base/parseint.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <meminfo/procmeminfo.h>
#include <meminfo/sysmeminfo.h>

#include <smapinfo.h>

namespace android {
namespace smapinfo {

using ::android::base::StringPrintf;
using ::android::meminfo::EscapeCsvString;
using ::android::meminfo::EscapeJsonString;
using ::android::meminfo::Format;
using ::android::meminfo::MemUsage;
using ::android::meminfo::ProcMemInfo;
using ::android::meminfo::Vma;

struct ProcessRecord {
  public:
    ProcessRecord(pid_t pid, bool get_wss, uint64_t pgflags, uint64_t pgflags_mask,
                  bool get_cmdline, bool get_oomadj, std::ostream& err)
        : pid_(-1),
          oomadj_(OOM_SCORE_ADJ_MAX + 1),
          proportional_swap_(0),
          unique_swap_(0),
          zswap_(0) {
        procmem_ = std::make_unique<ProcMemInfo>(pid, get_wss, pgflags, pgflags_mask);
        if (procmem_ == nullptr) {
            err << "Failed to create ProcMemInfo for: " << pid << "\n";
            return;
        }

        // cmdline_ only needs to be populated if this record will be used by procrank/librank.
        if (get_cmdline) {
            std::string fname = StringPrintf("/proc/%d/cmdline", pid);
            if (!::android::base::ReadFileToString(fname, &cmdline_)) {
                std::cerr << "Failed to read cmdline from: " << fname << "\n";
                cmdline_ = "<unknown>";
            }
            // We deliberately don't read the /proc/<pid>/cmdline file directly into 'cmdline_'
            // because some processes have cmdlines that end with "0x00 0x0A 0x00",
            // e.g. xtra-daemon, lowi-server.
            // The .c_str() assignment takes care of trimming the cmdline at the first 0x00. This is
            // how the original procrank worked (luckily).
            cmdline_.resize(strlen(cmdline_.c_str()));
        }

        // oomadj_ only needs to be populated if this record will be used by procrank/librank.
        if (get_oomadj) {
            std::string fname = StringPrintf("/proc/%d/oom_score_adj", pid);
            std::string oom_score;
            if (!::android::base::ReadFileToString(fname, &oom_score)) {
                std::cerr << "Failed to read oom_score_adj file: " << fname << "\n";
                return;
            }
            if (!::android::base::ParseInt(::android::base::Trim(oom_score), &oomadj_)) {
                std::cerr << "Failed to parse oomadj from: " << fname << "\n";
                return;
            }
        }

        // We want to use Smaps() to populate procmem_'s maps before calling Wss() or Usage(), as
        // these will fall back on the slower ReadMaps().
        procmem_->Smaps("", true);
        usage_or_wss_ = get_wss ? procmem_->Wss() : procmem_->Usage();
        swap_offsets_ = procmem_->SwapOffsets();
        pid_ = pid;
    }

    bool valid() const { return pid_ != -1; }

    void CalculateSwap(const std::vector<uint16_t>& swap_offset_array,
                       float zram_compression_ratio) {
        for (auto& off : swap_offsets_) {
            proportional_swap_ += getpagesize() / swap_offset_array[off];
            unique_swap_ += swap_offset_array[off] == 1 ? getpagesize() : 0;
            zswap_ = proportional_swap_ * zram_compression_ratio;
        }
        // This is divided by 1024 to convert to KB.
        proportional_swap_ /= 1024;
        unique_swap_ /= 1024;
        zswap_ /= 1024;
    }

    // Getters
    pid_t pid() const { return pid_; }
    const std::string& cmdline() const { return cmdline_; }
    int32_t oomadj() const { return oomadj_; }
    uint64_t proportional_swap() const { return proportional_swap_; }
    uint64_t unique_swap() const { return unique_swap_; }
    uint64_t zswap() const { return zswap_; }

    // Wrappers to ProcMemInfo
    const std::vector<uint64_t>& SwapOffsets() const { return swap_offsets_; }
    // show_wss may be used to return differentiated output in the future.
    const MemUsage& Usage([[maybe_unused]] bool show_wss) const { return usage_or_wss_; }

    // This will not result in a second reading of the smaps file because procmem_->Smaps() has
    // already been called in the constructor.
    const std::vector<Vma>& Smaps() const { return procmem_->Smaps(); }

  private:
    std::unique_ptr<ProcMemInfo> procmem_;
    pid_t pid_;
    std::string cmdline_;
    int32_t oomadj_;
    uint64_t proportional_swap_;
    uint64_t unique_swap_;
    uint64_t zswap_;
    MemUsage usage_or_wss_;
    std::vector<uint64_t> swap_offsets_;
};

bool get_all_pids(std::set<pid_t>* pids) {
    pids->clear();
    std::unique_ptr<DIR, int (*)(DIR*)> procdir(opendir("/proc"), closedir);
    if (!procdir) return false;

    struct dirent* dir;
    pid_t pid;
    while ((dir = readdir(procdir.get()))) {
        if (!::android::base::ParseInt(dir->d_name, &pid)) continue;
        pids->insert(pid);
    }
    return true;
}

namespace procrank {

static bool count_swap_offsets(const ProcessRecord& proc, std::vector<uint16_t>& swap_offset_array,
                               std::ostream& err) {
    const std::vector<uint64_t>& swp_offs = proc.SwapOffsets();
    for (auto& off : swp_offs) {
        if (off >= swap_offset_array.size()) {
            err << "swap offset " << off << " is out of bounds for process: " << proc.pid() << "\n";
            return false;
        }
        if (swap_offset_array[off] == USHRT_MAX) {
            err << "swap offset " << off << " ref count overflow in process: " << proc.pid()
                << "\n";
            return false;
        }
        swap_offset_array[off]++;
    }
    return true;
}

struct params {
    // Calculated total memory usage across all processes in the system.
    uint64_t total_pss;
    uint64_t total_uss;
    uint64_t total_swap;
    uint64_t total_pswap;
    uint64_t total_uswap;
    uint64_t total_zswap;

    // Print options.
    bool show_oomadj;
    bool show_wss;
    bool swap_enabled;
    bool zram_enabled;

    // If zram is enabled, the compression ratio is zram used / swap used.
    float zram_compression_ratio;
};

static std::function<bool(ProcessRecord& a, ProcessRecord& b)> select_sort(struct params* params,
                                                                           SortOrder sort_order) {
    // Create sort function based on sort_order.
    std::function<bool(ProcessRecord & a, ProcessRecord & b)> proc_sort;
    switch (sort_order) {
        case (SortOrder::BY_OOMADJ):
            proc_sort = [](ProcessRecord& a, ProcessRecord& b) { return a.oomadj() > b.oomadj(); };
            break;
        case (SortOrder::BY_RSS):
            proc_sort = [=](ProcessRecord& a, ProcessRecord& b) {
                return a.Usage(params->show_wss).rss > b.Usage(params->show_wss).rss;
            };
            break;
        case (SortOrder::BY_SWAP):
            proc_sort = [=](ProcessRecord& a, ProcessRecord& b) {
                return a.Usage(params->show_wss).swap > b.Usage(params->show_wss).swap;
            };
            break;
        case (SortOrder::BY_USS):
            proc_sort = [=](ProcessRecord& a, ProcessRecord& b) {
                return a.Usage(params->show_wss).uss > b.Usage(params->show_wss).uss;
            };
            break;
        case (SortOrder::BY_VSS):
            proc_sort = [=](ProcessRecord& a, ProcessRecord& b) {
                return a.Usage(params->show_wss).vss > b.Usage(params->show_wss).vss;
            };
            break;
        case (SortOrder::BY_PSS):
        default:
            proc_sort = [=](ProcessRecord& a, ProcessRecord& b) {
                return a.Usage(params->show_wss).pss > b.Usage(params->show_wss).pss;
            };
            break;
    }
    return proc_sort;
}

static bool populate_procs(struct params* params, uint64_t pgflags, uint64_t pgflags_mask,
                           std::vector<uint16_t>& swap_offset_array, const std::set<pid_t>& pids,
                           std::vector<ProcessRecord>* procs, std::ostream& err) {
    // Mark each swap offset used by the process as we find them for calculating
    // proportional swap usage later.
    for (pid_t pid : pids) {
        ProcessRecord proc(pid, params->show_wss, pgflags, pgflags_mask, true, params->show_oomadj,
                           err);

        if (!proc.valid()) {
            // Check to see if the process is still around, skip the process if the proc
            // directory is inaccessible. It was most likely killed while creating the process
            // record.
            std::string procdir = StringPrintf("/proc/%d", pid);
            if (access(procdir.c_str(), F_OK | R_OK)) continue;

            // Warn if we failed to gather process stats even while it is still alive.
            // Return success here, so we continue to print stats for other processes.
            err << "warning: failed to create process record for: " << pid << "\n";
            continue;
        }

        // Skip processes with no memory mappings.
        uint64_t vss = proc.Usage(params->show_wss).vss;
        if (vss == 0) continue;

        // Collect swap_offset counts from all processes in 1st pass.
        if (!params->show_wss && params->swap_enabled &&
            !count_swap_offsets(proc, swap_offset_array, err)) {
            err << "Failed to count swap offsets for process: " << pid << "\n";
            err << "Failed to read all pids from the system\n";
            return false;
        }

        procs->emplace_back(std::move(proc));
    }
    return true;
}

static void print_header(struct params* params, std::ostream& out) {
    out << StringPrintf("%5s  ", "PID");
    if (params->show_oomadj) {
        out << StringPrintf("%5s  ", "oom");
    }

    if (params->show_wss) {
        out << StringPrintf("%7s  %7s  %7s  ", "WRss", "WPss", "WUss");
    } else {
        // Swap statistics here, as working set pages by definition shouldn't end up in swap.
        out << StringPrintf("%8s  %7s  %7s  %7s  ", "Vss", "Rss", "Pss", "Uss");
        if (params->swap_enabled) {
            out << StringPrintf("%7s  %7s  %7s  ", "Swap", "PSwap", "USwap");
            if (params->zram_enabled) {
                out << StringPrintf("%7s  ", "ZSwap");
            }
        }
    }

    out << "cmdline\n";
}

static void print_divider(struct params* params, std::ostream& out) {
    out << StringPrintf("%5s  ", "");
    if (params->show_oomadj) {
        out << StringPrintf("%5s  ", "");
    }

    if (params->show_wss) {
        out << StringPrintf("%7s  %7s  %7s  ", "", "------", "------");
    } else {
        out << StringPrintf("%8s  %7s  %7s  %7s  ", "", "", "------", "------");
        if (params->swap_enabled) {
            out << StringPrintf("%7s  %7s  %7s  ", "------", "------", "------");
            if (params->zram_enabled) {
                out << StringPrintf("%7s  ", "------");
            }
        }
    }

    out << StringPrintf("%s\n", "------");
}

static void print_processrecord(struct params* params, ProcessRecord& proc, std::ostream& out) {
    out << StringPrintf("%5d  ", proc.pid());
    if (params->show_oomadj) {
        out << StringPrintf("%5d  ", proc.oomadj());
    }

    if (params->show_wss) {
        out << StringPrintf("%6" PRIu64 "K  %6" PRIu64 "K  %6" PRIu64 "K  ",
                            proc.Usage(params->show_wss).rss, proc.Usage(params->show_wss).pss,
                            proc.Usage(params->show_wss).uss);
    } else {
        out << StringPrintf("%7" PRIu64 "K  %6" PRIu64 "K  %6" PRIu64 "K  %6" PRIu64 "K  ",
                            proc.Usage(params->show_wss).vss, proc.Usage(params->show_wss).rss,
                            proc.Usage(params->show_wss).pss, proc.Usage(params->show_wss).uss);
        if (params->swap_enabled) {
            out << StringPrintf("%6" PRIu64 "K  ", proc.Usage(params->show_wss).swap);
            out << StringPrintf("%6" PRIu64 "K  ", proc.proportional_swap());
            out << StringPrintf("%6" PRIu64 "K  ", proc.unique_swap());
            if (params->zram_enabled) {
                out << StringPrintf("%6" PRIu64 "K  ", proc.zswap());
            }
        }
    }
    out << proc.cmdline() << "\n";
}

static void print_totals(struct params* params, std::ostream& out) {
    out << StringPrintf("%5s  ", "");
    if (params->show_oomadj) {
        out << StringPrintf("%5s  ", "");
    }

    if (params->show_wss) {
        out << StringPrintf("%7s  %6" PRIu64 "K  %6" PRIu64 "K  ", "", params->total_pss,
                            params->total_uss);
    } else {
        out << StringPrintf("%8s  %7s  %6" PRIu64 "K  %6" PRIu64 "K  ", "", "", params->total_pss,
                            params->total_uss);
        if (params->swap_enabled) {
            out << StringPrintf("%6" PRIu64 "K  ", params->total_swap);
            out << StringPrintf("%6" PRIu64 "K  ", params->total_pswap);
            out << StringPrintf("%6" PRIu64 "K  ", params->total_uswap);
            if (params->zram_enabled) {
                out << StringPrintf("%6" PRIu64 "K  ", params->total_zswap);
            }
        }
    }
    out << "TOTAL\n\n";
}

static void print_sysmeminfo(struct params* params, const ::android::meminfo::SysMemInfo& smi,
                             std::ostream& out) {
    if (params->swap_enabled) {
        out << StringPrintf("ZRAM: %" PRIu64 "K physical used for %" PRIu64 "K in swap (%" PRIu64
                            "K total swap)\n",
                            smi.mem_zram_kb(), (smi.mem_swap_kb() - smi.mem_swap_free_kb()),
                            smi.mem_swap_kb());
    }

    out << StringPrintf(" RAM: %" PRIu64 "K total, %" PRIu64 "K free, %" PRIu64
                        "K buffers, %" PRIu64 "K cached, %" PRIu64 "K shmem, %" PRIu64 "K slab\n",
                        smi.mem_total_kb(), smi.mem_free_kb(), smi.mem_buffers_kb(),
                        smi.mem_cached_kb(), smi.mem_shmem_kb(), smi.mem_slab_kb());
}

static void add_to_totals(struct params* params, ProcessRecord& proc,
                          const std::vector<uint16_t>& swap_offset_array) {
    params->total_pss += proc.Usage(params->show_wss).pss;
    params->total_uss += proc.Usage(params->show_wss).uss;
    if (!params->show_wss && params->swap_enabled) {
        proc.CalculateSwap(swap_offset_array, params->zram_compression_ratio);
        params->total_swap += proc.Usage(params->show_wss).swap;
        params->total_pswap += proc.proportional_swap();
        params->total_uswap += proc.unique_swap();
        if (params->zram_enabled) {
            params->total_zswap += proc.zswap();
        }
    }
}

}  // namespace procrank

bool run_procrank(uint64_t pgflags, uint64_t pgflags_mask, const std::set<pid_t>& pids,
                  bool get_oomadj, bool get_wss, SortOrder sort_order, bool reverse_sort,
                  std::ostream& out, std::ostream& err) {
    ::android::meminfo::SysMemInfo smi;
    if (!smi.ReadMemInfo()) {
        err << "Failed to get system memory info\n";
        return false;
    }

    struct procrank::params params = {
            .total_pss = 0,
            .total_uss = 0,
            .total_swap = 0,
            .total_pswap = 0,
            .total_uswap = 0,
            .total_zswap = 0,
            .show_oomadj = get_oomadj,
            .show_wss = get_wss,
            .swap_enabled = false,
            .zram_enabled = false,
            .zram_compression_ratio = 0.0,
    };

    // Figure out swap and zram.
    uint64_t swap_total = smi.mem_swap_kb() * 1024;
    params.swap_enabled = swap_total > 0;
    // Allocate the swap array.
    std::vector<uint16_t> swap_offset_array(swap_total / getpagesize() + 1, 0);
    if (params.swap_enabled) {
        params.zram_enabled = smi.mem_zram_kb() > 0;
        if (params.zram_enabled) {
            params.zram_compression_ratio = static_cast<float>(smi.mem_zram_kb()) /
                                            (smi.mem_swap_kb() - smi.mem_swap_free_kb());
        }
    }

    std::vector<ProcessRecord> procs;
    if (!procrank::populate_procs(&params, pgflags, pgflags_mask, swap_offset_array, pids, &procs,
                                  err)) {
        return false;
    }

    if (procs.empty()) {
        // This would happen in corner cases where procrank is being run to find KSM usage on a
        // system with no KSM and combined with working set determination as follows
        //   procrank -w -u -k
        //   procrank -w -s -k
        //   procrank -w -o -k
        out << "<empty>\n\n";
        procrank::print_sysmeminfo(&params, smi, out);
        return true;
    }

    // Create sort function based on sort_order, default is PSS descending.
    std::function<bool(ProcessRecord & a, ProcessRecord & b)> proc_sort =
            procrank::select_sort(&params, sort_order);

    // Sort all process records, default is PSS descending.
    if (reverse_sort) {
        std::sort(procs.rbegin(), procs.rend(), proc_sort);
    } else {
        std::sort(procs.begin(), procs.end(), proc_sort);
    }

    procrank::print_header(&params, out);

    for (auto& proc : procs) {
        procrank::add_to_totals(&params, proc, swap_offset_array);
        procrank::print_processrecord(&params, proc, out);
    }

    procrank::print_divider(&params, out);
    procrank::print_totals(&params, out);
    procrank::print_sysmeminfo(&params, smi, out);

    return true;
}

namespace librank {

static void add_mem_usage(MemUsage* to, const MemUsage& from) {
    to->vss += from.vss;
    to->rss += from.rss;
    to->pss += from.pss;
    to->uss += from.uss;

    to->swap += from.swap;

    to->private_clean += from.private_clean;
    to->private_dirty += from.private_dirty;
    to->shared_clean += from.shared_clean;
    to->shared_dirty += from.shared_dirty;
}

// Represents a specific process's usage of a library.
struct LibProcRecord {
  public:
    LibProcRecord(ProcessRecord& proc) : pid_(-1), oomadj_(OOM_SCORE_ADJ_MAX + 1) {
        pid_ = proc.pid();
        cmdline_ = proc.cmdline();
        oomadj_ = proc.oomadj();
        usage_.clear();
    }

    bool valid() const { return pid_ != -1; }
    void AddUsage(const MemUsage& mem_usage) { add_mem_usage(&usage_, mem_usage); }

    // Getters
    pid_t pid() const { return pid_; }
    const std::string& cmdline() const { return cmdline_; }
    int32_t oomadj() const { return oomadj_; }
    const MemUsage& usage() const { return usage_; }

  private:
    pid_t pid_;
    std::string cmdline_;
    int32_t oomadj_;
    MemUsage usage_;
};

// Represents all processes' usage of a specific library.
struct LibRecord {
  public:
    LibRecord(const std::string& name) : name_(name) {}

    void AddUsage(const LibProcRecord& proc, const MemUsage& mem_usage) {
        auto [it, inserted] = procs_.insert(std::pair<pid_t, LibProcRecord>(proc.pid(), proc));
        // Adds to proc's PID's contribution to usage of this lib, as well as total lib usage.
        it->second.AddUsage(mem_usage);
        add_mem_usage(&usage_, mem_usage);
    }
    uint64_t pss() const { return usage_.pss; }

    // Getters
    const std::string& name() const { return name_; }
    const std::map<pid_t, LibProcRecord>& processes() const { return procs_; }

  private:
    std::string name_;
    MemUsage usage_;
    std::map<pid_t, LibProcRecord> procs_;
};

static std::function<bool(LibProcRecord& a, LibProcRecord& b)> select_sort(SortOrder sort_order) {
    // Create sort function based on sort_order.
    std::function<bool(LibProcRecord & a, LibProcRecord & b)> proc_sort;
    switch (sort_order) {
        case (SortOrder::BY_RSS):
            proc_sort = [](LibProcRecord& a, LibProcRecord& b) {
                return a.usage().rss > b.usage().rss;
            };
            break;
        case (SortOrder::BY_USS):
            proc_sort = [](LibProcRecord& a, LibProcRecord& b) {
                return a.usage().uss > b.usage().uss;
            };
            break;
        case (SortOrder::BY_VSS):
            proc_sort = [](LibProcRecord& a, LibProcRecord& b) {
                return a.usage().vss > b.usage().vss;
            };
            break;
        case (SortOrder::BY_OOMADJ):
            proc_sort = [](LibProcRecord& a, LibProcRecord& b) { return a.oomadj() > b.oomadj(); };
            break;
        case (SortOrder::BY_PSS):
        default:
            proc_sort = [](LibProcRecord& a, LibProcRecord& b) {
                return a.usage().pss > b.usage().pss;
            };
            break;
    }
    return proc_sort;
}

struct params {
    // Filtering options.
    std::string lib_prefix;
    bool all_libs;
    const std::vector<std::string>& excluded_libs;
    uint16_t mapflags_mask;

    // Print options.
    Format format;
    bool swap_enabled;
    bool show_oomadj;
};

static bool populate_libs(struct params* params, uint64_t pgflags, uint64_t pgflags_mask,
                          const std::set<pid_t>& pids,
                          std::map<std::string, LibRecord>& lib_name_map, std::ostream& err) {
    for (pid_t pid : pids) {
        ProcessRecord proc(pid, false, pgflags, pgflags_mask, true, params->show_oomadj, err);
        if (!proc.valid()) {
            err << "error: failed to create process record for: " << pid << "\n";
            return false;
        }

        const std::vector<Vma>& maps = proc.Smaps();
        if (maps.size() == 0) {
            continue;
        }

        LibProcRecord record(proc);
        for (const Vma& map : maps) {
            // Skip library/map if the prefix for the path doesn't match.
            if (!params->lib_prefix.empty() &&
                !::android::base::StartsWith(map.name, params->lib_prefix)) {
                continue;
            }
            // Skip excluded library/map names.
            if (!params->all_libs &&
                (std::find(params->excluded_libs.begin(), params->excluded_libs.end(), map.name) !=
                 params->excluded_libs.end())) {
                continue;
            }
            // Skip maps based on map permissions.
            if (params->mapflags_mask &&
                ((map.flags & (PROT_READ | PROT_WRITE | PROT_EXEC)) != params->mapflags_mask)) {
                continue;
            }

            // Add memory for lib usage.
            auto [it, inserted] = lib_name_map.emplace(map.name, LibRecord(map.name));
            it->second.AddUsage(record, map.usage);

            if (!params->swap_enabled && map.usage.swap) {
                params->swap_enabled = true;
            }
        }
    }
    return true;
}

static void print_header(struct params* params, std::ostream& out) {
    switch (params->format) {
        case Format::RAW:
            // clang-format off
            out << std::setw(7) << "RSStot"
                << std::setw(10) << "VSS"
                << std::setw(9) << "RSS"
                << std::setw(9) << "PSS"
                << std::setw(9) << "USS"
                << "  ";
            //clang-format on
            if (params->swap_enabled) {
                out << std::setw(7) << "Swap"
                    << "  ";
            }
            if (params->show_oomadj) {
                out << std::setw(7) << "Oom"
                    << "  ";
            }
            out << "Name/PID\n";
            break;
        case Format::CSV:
            out << "\"Library\",\"Total_RSS\",\"Process\",\"PID\",\"VSS\",\"RSS\",\"PSS\",\"USS\"";
            if (params->swap_enabled) {
                out << ",\"Swap\"";
            }
            if (params->show_oomadj) {
                out << ",\"Oomadj\"";
            }
            out << "\n";
            break;
        case Format::JSON:
        default:
            break;
    }
}

static void print_library(struct params* params, const LibRecord& lib,
                          std::ostream& out) {
    if (params->format == Format::RAW) {
        // clang-format off
        out << std::setw(6) << lib.pss() << "K"
            << std::setw(10) << ""
            << std::setw(9) << ""
            << std::setw(9) << ""
            << std::setw(9) << ""
            << "  ";
        // clang-format on
        if (params->swap_enabled) {
            out << std::setw(7) << ""
                << "  ";
        }
        if (params->show_oomadj) {
            out << std::setw(7) << ""
                << "  ";
        }
        out << lib.name() << "\n";
    }
}

static void print_proc_as_raw(struct params* params, const LibProcRecord& p, std::ostream& out) {
    const MemUsage& usage = p.usage();
    // clang-format off
    out << std::setw(7) << ""
        << std::setw(9) << usage.vss << "K  "
        << std::setw(6) << usage.rss << "K  "
        << std::setw(6) << usage.pss << "K  "
        << std::setw(6) << usage.uss << "K  ";
    // clang-format on
    if (params->swap_enabled) {
        out << std::setw(6) << usage.swap << "K  ";
    }
    if (params->show_oomadj) {
        out << std::setw(7) << p.oomadj() << "  ";
    }
    out << "  " << p.cmdline() << " [" << p.pid() << "]\n";
}

static void print_proc_as_json(struct params* params, const LibRecord& l, const LibProcRecord& p,
                               std::ostream& out) {
    const MemUsage& usage = p.usage();
    // clang-format off
    out << "{\"Library\":" << EscapeJsonString(l.name())
        << ",\"Total_RSS\":" << l.pss()
        << ",\"Process\":" << EscapeJsonString(p.cmdline())
        << ",\"PID\":\"" << p.pid() << "\""
        << ",\"VSS\":" << usage.vss
        << ",\"RSS\":" << usage.rss
        << ",\"PSS\":" << usage.pss
        << ",\"USS\":" << usage.uss;
    // clang-format on
    if (params->swap_enabled) {
        out << ",\"Swap\":" << usage.swap;
    }
    if (params->show_oomadj) {
        out << ",\"Oom\":" << p.oomadj();
    }
    out << "}\n";
}

static void print_proc_as_csv(struct params* params, const LibRecord& l, const LibProcRecord& p,
                              std::ostream& out) {
    const MemUsage& usage = p.usage();
    // clang-format off
    out << EscapeCsvString(l.name())
        << "," << l.pss()
        << "," << EscapeCsvString(p.cmdline())
        << ",\"[" << p.pid() << "]\""
        << "," << usage.vss
        << "," << usage.rss
        << "," << usage.pss
        << "," << usage.uss;
    // clang-format on
    if (params->swap_enabled) {
        out << "," << usage.swap;
    }
    if (params->show_oomadj) {
        out << "," << p.oomadj();
    }
    out << "\n";
}

static void print_procs(struct params* params, const LibRecord& lib,
                        const std::vector<LibProcRecord>& procs, std::ostream& out) {
    for (const LibProcRecord& p : procs) {
        switch (params->format) {
            case Format::RAW:
                print_proc_as_raw(params, p, out);
                break;
            case Format::JSON:
                print_proc_as_json(params, lib, p, out);
                break;
            case Format::CSV:
                print_proc_as_csv(params, lib, p, out);
                break;
            default:
                break;
        }
    }
}

}  // namespace librank

bool run_librank(uint64_t pgflags, uint64_t pgflags_mask, const std::set<pid_t>& pids,
                 const std::string& lib_prefix, bool all_libs,
                 const std::vector<std::string>& excluded_libs, uint16_t mapflags_mask,
                 Format format, SortOrder sort_order, bool reverse_sort, std::ostream& out,
                 std::ostream& err) {
    struct librank::params params = {
            .lib_prefix = lib_prefix,
            .all_libs = all_libs,
            .excluded_libs = excluded_libs,
            .mapflags_mask = mapflags_mask,
            .format = format,
            .swap_enabled = false,
            .show_oomadj = (sort_order == SortOrder::BY_OOMADJ),
    };

    // Fills in usage info for each LibRecord.
    std::map<std::string, librank::LibRecord> lib_name_map;
    if (!librank::populate_libs(&params, pgflags, pgflags_mask, pids, lib_name_map, err)) {
        return false;
    }

    librank::print_header(&params, out);

    // Create vector of all LibRecords, sorted by descending PSS.
    std::vector<librank::LibRecord> libs;
    libs.reserve(lib_name_map.size());
    for (const auto& [k, v] : lib_name_map) {
        libs.push_back(v);
    }
    std::sort(libs.begin(), libs.end(),
              [](const librank::LibRecord& l1, const librank::LibRecord& l2) {
                  return l1.pss() > l2.pss();
              });

    std::function<bool(librank::LibProcRecord & a, librank::LibProcRecord & b)> libproc_sort =
            librank::select_sort(sort_order);
    for (librank::LibRecord& lib : libs) {
        // Sort all processes for this library, default is PSS-descending.
        std::vector<librank::LibProcRecord> procs;
        procs.reserve(lib.processes().size());
        for (const auto& [k, v] : lib.processes()) {
            procs.push_back(v);
        }
        if (reverse_sort) {
            std::sort(procs.rbegin(), procs.rend(), libproc_sort);
        } else {
            std::sort(procs.begin(), procs.end(), libproc_sort);
        }

        librank::print_library(&params, lib, out);
        librank::print_procs(&params, lib, procs, out);
    }

    return true;
}

}  // namespace smapinfo
}  // namespace android
