#![allow(unused)]

use std::collections::HashMap;
use std::thread;
use std::sync::{Arc, RwLock};

fn main() {
	println!("\n[There should be only one value in the hashmap. But this code has atomicity violation which results in multiple values.]\n");
	let mut hash = Arc::new(RwLock::new(HashMap::new()));
	hash.write().unwrap().insert(1, "initial data".to_string());
	let mut hash2 = hash.clone();

	let handler = thread::spawn(move || {
		hash2.write().unwrap().clear();
		hash2.write().unwrap().insert(2, "second data".to_string());
	});

	hash.write().unwrap().clear();
	handler.join();
	hash.write().unwrap().insert(3, "third data".to_string());

	for value in hash.read().unwrap().values() {
		println!("value: {}", value);
	}
}

