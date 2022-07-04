#![allow(warnings)]
use std::thread;
use std::sync::{Arc, RwLock};

fn main() {
	let lock = Arc::new(RwLock::new(5));
	println!("lock address: {:p}", &lock);

	// many reader locks can be held at once
	{
		 let r1 = lock.read().unwrap();
		 let r2 = lock.read().unwrap();
		 println!("r1 address: {:p}", &r1);
		 println!("r2 address: {:p}", &r2);
	} // read locks are dropped at this point

	let lock2 = lock.clone();
	// only one write lock may be held, however
	let handler = thread::spawn(move || {
		 let mut w1 = lock2.write().unwrap();
		 println!("w1 address: {:p}", &w1);
	} // write lock is dropped here
	);
	handler.join();
}
