use std::fs::File;
use std::io::prelude::*;

let mut file: Result<File> = None;

pub fn _final_() {
	file.write_all("---------------------From _final_---------------------\n");

}

pub fn _init_() {
	file = File::create("log");
	file.write_all("---------------------From _init_---------------------\n");
}

pub fn _probe_(line: i32, var: *char, op: char*, val: i32) {
	file.write_all("---------------------From _probe_---------------------\n");
}
