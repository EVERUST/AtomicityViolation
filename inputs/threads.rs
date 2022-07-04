#![allow(warnings)]
use std::sync::{Mutex, Arc};
use std::thread;

fn main() {
	let m1 = Arc::new(Mutex::new(0));
	let m2 = m1.clone();

	let handler = thread::spawn(move || {
		{
		let d2 = m2.lock().unwrap();
		}
	});

	handler.join();
	m1.lock().unwrap();
}
