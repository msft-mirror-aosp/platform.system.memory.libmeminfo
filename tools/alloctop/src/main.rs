/*
 * Copyright (C) 2024 The Android Open Source Project
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

//! Tool to help the parsing and filtering of /proc/allocinfo

use std::collections::HashMap;
use std::env;
use std::fs::File;
use std::io::{self, BufRead, BufReader};
use std::process;

const PROC_ALLOCINFO: &str = "/proc/allocinfo";

#[derive(Debug, PartialEq, Eq)]
struct AllocInfo {
    size: u64,
    calls: u64,
    tag: String,
}

struct AllocGlobal {
    size: u64,
    calls: u64,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum SortBy {
    Size,
    Calls,
    Tag,
}

fn print_help() {
    println!("alloctop - A tool for analyzing memory allocations from /proc/allocinfo\n");
    println!("Usage: alloctop [OPTIONS]\n");
    println!("Options:");
    println!("  -m, --min <size>    Only display allocations with size greater than <size>");
    println!("  -n, --lines <num>   Only output the first <num> lines");
    println!("  -o, --once          Display the output once and then exit.");
    println!("  -s, --sort <s|c|t>  Sort the output by size (s), number of calls (c), or tag (t)");
    println!("  -t, --tree          Aggregate output data by tag components. Only the \"min\"");
    println!("                      option is implemented for this visualization\n");
    println!("  -h, --help          Display this help message and exit");
}

#[cfg(unix)]
fn reset_sigpipe() {
    // SAFETY:
    // This is safe because we are simply resetting the SIGPIPE signal handler to its default behavior.
    // The `signal` function itself is marked as unsafe because it can globally modify process state.
    // However, in this specific case, we are restoring the default behavior of ignoring the signal
    // which is a well-defined and safe operation.
    unsafe {
        libc::signal(libc::SIGPIPE, libc::SIG_DFL);
    }
}

#[cfg(not(unix))]
fn reset_sigpipe() {
    // no-op
}

fn parse_allocinfo(filename: &str) -> io::Result<Vec<AllocInfo>> {
    let file = File::open(filename)?;
    let reader = BufReader::new(file);
    let mut alloc_info_list = Vec::new();

    for line in reader.lines() {
        let line = line?;
        let fields: Vec<&str> = line.split_whitespace().collect();

        if fields.len() >= 3 && fields[0] != "#" {
            let size = fields[0].parse::<u64>().unwrap_or(0);
            let calls = fields[1].parse::<u64>().unwrap_or(0);
            let tag = fields[2..].join(" ");

            // One possible implementation would be to check for the minimum size here, but skipping
            // lines at parsing time won't give correct results when the data is aggregated (e.g., for
            // tree view).
            alloc_info_list.push(AllocInfo { size, calls, tag });
        }
    }

    Ok(alloc_info_list)
}

fn sort_allocinfo(data: &mut [AllocInfo], sort_by: SortBy) {
    match sort_by {
        SortBy::Size => data.sort_by(|a, b| b.size.cmp(&a.size)),
        SortBy::Calls => data.sort_by(|a, b| b.calls.cmp(&a.calls)),
        SortBy::Tag => data.sort_by(|a, b| a.tag.cmp(&b.tag)),
    }
}

fn aggregate_tree(data: &[AllocInfo]) -> HashMap<String, (u64, u64)> {
    let mut aggregated_data: HashMap<String, (u64, u64)> = HashMap::new();

    for info in data {
        let parts: Vec<&str> = info.tag.split('/').collect();
        for i in 0..parts.len() {
            let tag_prefix = parts[..=i].join("/");
            let entry = aggregated_data.entry(tag_prefix).or_insert((0, 0));
            entry.0 += info.size;
            entry.1 += info.calls;
        }
    }

    aggregated_data
}

fn print_tree_data(data: &HashMap<String, (u64, u64)>, min_size: u64) {
    let mut sorted_data: Vec<_> = data.iter().collect();
    sorted_data.sort_by(|a, b| a.0.cmp(b.0));

    println!("{:>10} {:>10} Tag", "Size", "Calls");
    for (tag, (size, calls)) in sorted_data {
        if *size < min_size {
            continue;
        }
        println!("{:>10} {:>10} {}", size, calls, tag);
    }
}

fn aggregate_global(data: &[AllocInfo]) -> AllocGlobal {
    let mut globals = AllocGlobal { size: 0, calls: 0 };

    for info in data {
        globals.size += info.size;
        globals.calls += info.calls;
    }

    globals
}

fn print_aggregated_global_data(data: &AllocGlobal) {
    println!("{:>11} : {}", "Total Size", data.size);
    println!("{:>11} : {}\n", "Total Calls", data.calls);
}

fn main() {
    reset_sigpipe();

    let args: Vec<String> = env::args().collect();
    let mut max_lines: usize = usize::MAX;
    let mut sort_by = None;
    let mut min_size = 0;
    let mut use_tree = false;
    let mut display_once = false;

    let mut i = 1;
    while i < args.len() {
        match args[i].as_str() {
            "-h" | "--help" => {
                print_help();
                process::exit(0);
            }
            "-s" | "--sort" => {
                i += 1;
                if i < args.len() {
                    sort_by = match args[i].as_str() {
                        "s" => Some(SortBy::Size),
                        "c" => Some(SortBy::Calls),
                        "t" => Some(SortBy::Tag),
                        _ => {
                            eprintln!("Invalid sort option. Use 's', 'c', or 't'.");
                            process::exit(1);
                        }
                    };
                } else {
                    eprintln!("Missing argument for --sort.");
                    process::exit(1);
                }
            }
            "-m" | "--min" => {
                i += 1;
                if i < args.len() {
                    min_size = match args[i].parse::<u64>() {
                        Ok(val) => val,
                        Err(_) => {
                            eprintln!("Invalid minimum size. Please provide a valid number.");
                            process::exit(1);
                        }
                    };
                } else {
                    eprintln!("Missing argument for --min.");
                    process::exit(1);
                }
            }
            "-n" | "--lines" => {
                i += 1;
                if i < args.len() {
                    max_lines = match args[i].parse::<usize>() {
                        Ok(val) => val,
                        Err(_) => {
                            eprintln!("Invalid lines. Please provide a valid number.");
                            process::exit(1);
                        }
                    };
                } else {
                    eprintln!("Missing argument for --lines.");
                    process::exit(1);
                }
            }
            "-o" | "--once" => {
                display_once = true;
            }
            "-t" | "--tree" => {
                use_tree = true;
            }
            _ => {
                eprintln!("Invalid argument: {}", args[i]);
                print_help();
                process::exit(1);
            }
        }
        i += 1;
    }

    if !display_once {
        eprintln!("Only \"display once\" mode currently available, run with \"-o\".");
        process::exit(1);
    }

    match parse_allocinfo(PROC_ALLOCINFO) {
        Ok(mut data) => {
            {
                let aggregated_data = aggregate_global(&data);
                print_aggregated_global_data(&aggregated_data);
            }

            if use_tree {
                let tree_data = aggregate_tree(&data);
                print_tree_data(&tree_data, min_size);
            } else {
                data.retain(|alloc_info| alloc_info.size >= min_size);

                if let Some(sort_by) = sort_by {
                    sort_allocinfo(&mut data, sort_by);
                }

                let printable_lines = if max_lines <= data.len() { max_lines } else { data.len() };
                println!("{:>10} {:>10} Tag", "Size", "Calls");
                for info in &data[0..printable_lines] {
                    println!("{:>10} {:>10} {}", info.size, info.calls, info.tag);
                }
            }
        }
        Err(e) => {
            eprintln!("Error reading or parsing allocinfo: {}", e);
            process::exit(1);
        }
    }
}
