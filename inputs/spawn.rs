#![allow(warnings)]
use std::thread;

fn main() {
	let handler = thread::spawn(|| {
		println!("inner thread");
	});
	handler.join();
	println!("outer thread");
}
