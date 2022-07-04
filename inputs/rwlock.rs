#![allow(warnings)]
use std::sync::RwLock;

fn main() {
	let lock = RwLock::new(5);
	println!("lock address: {:p}", &lock);

	// many reader locks can be held at once
	{
		 let r1 = lock.read().unwrap();
		 let r2 = lock.read().unwrap();
		 println!("r1 address: {:p}", &r1);
		 println!("r2 address: {:p}", &r2);
	} // read locks are dropped at this point

	// only one write lock may be held, however
	{
		 let mut w = lock.write().unwrap();
		 println!("w address: {:p}", &w);
	} // write lock is dropped here
}
