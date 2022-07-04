#![allow(unused)]
#![allow(deprecated)]

fn main() {
	let concbag = Arc::new(ConcBag {
		head: Arc::new(AtomicPtr::new(ptr::null_mut())),
	});

	for i in 0..5 {
		concbag.insert(i.to_string());
	}

	println!("------------------------------");
	print_concbag(&concbag, "initial");
	println!("------------------------------");

	let thread_concbag = concbag.clone();
	let handler = thread::spawn(move || {
		unsafe { thread_concbag.collect() };
	});
	
	thread::sleep(Duration::from_secs(1));
	println!("insert 5");
	concbag.insert(String::from("5"));

	handler.join();
	print_concbag(&concbag, "main thread after join");
}

fn print_concbag(concbag: &ConcBag, caller: &str) {
	println!("printing - {}", caller);
	let mut node = concbag.head.load(SeqCst);
	while node != ptr::null_mut() {
		println!("{} - {}", caller, unsafe { &(*node).data });
		unsafe { node = (*node).next.load(SeqCst) };
	}
}

use std::ptr;
use std::sync::atomic::AtomicPtr;
use std::sync::atomic::Ordering::{Relaxed, Release, SeqCst, Acquire};

use std::time::Duration;
use std::thread;
use std::sync::Arc;

#[derive(Clone)]
struct ConcBag {
	head: Arc<AtomicPtr<Node>>,
}

struct Item {
	ptr: *mut u8,
	free: unsafe fn(*mut u8),
}

struct Node {
	data: String,
	next: AtomicPtr<Node>,
}

impl ConcBag {
	fn insert(&self, t: String) {
		let n = Box::into_raw(Box::new(
			Node { data: t, next: AtomicPtr::new( ptr::null_mut()) }));
		loop {
			let head = self.head.load(Relaxed);
			unsafe { (*n).next.store(head, Relaxed) };
			if self.head.compare_and_swap(head, n, Release) == head { break }
		}
	}

	unsafe fn collect(&self) {
		println!("collecting...");
		let mut head = self.head.load(Relaxed);
		
		thread::sleep(Duration::from_secs(2));

		self.head.store(ptr::null_mut(), Relaxed);
		
		// changed code
		/*
		if head != ptr::null_mut() {
			head = self.head.swap(ptr::null_mut(), Acquire);
			while head != ptr::null_mut() {
				let mut n = Box::from_raw(head);
				println!("collecting - {}", (*head).data );
				head = n.next.load(Relaxed);
			}
		}
		*/
		
		// error code
		while head != ptr::null_mut() {
			let mut n = Box::from_raw(head);
			println!("collecting - {}", (*head).data );
			head = n.next.load(Relaxed);
		}
		
		println!("collected");
	}
}

