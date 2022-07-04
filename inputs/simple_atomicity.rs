#![allow(warnings)]
use std::thread;
use std::sync::{Arc, Mutex};

struct B {test: i32}

fn main() {
	let mut var = Arc::new(Mutex::new(B{test: 3}));
	let v1 = var.lock().unwrap().test;
	println!("var lock: {:p}", Arc::as_ptr(&var));
	println!("var.test: {:p}", &(var.lock().unwrap().test));

	let mut var2 = var.clone();

	let handle = thread::spawn(move || {
		var2.lock().unwrap().test = 5;
	});
	handle.join();
	let v3 = var.lock().unwrap().test;
}
