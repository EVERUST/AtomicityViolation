#![allow(warnings)]
use std::sync::atomic::*;
use std::sync::atomic::Ordering::{Relaxed, Release, SeqCst, Acquire};

fn main() {
	let sc = SeqCst;

	let ab = AtomicBool::new(true);

	let aI8 = AtomicI8::new(-8);
	let aI16 = AtomicI16::new(-16);
	let aI32 = AtomicI32::new(-32);
	let aI64 = AtomicI64::new(-64);
	let aIsize = AtomicIsize::new(-88);

	let aPtr = AtomicPtr::new(&mut 6);

	let aU8 = AtomicU8::new(8);
	let aU16 = AtomicU16::new(16);
	let aU32 = AtomicU32::new(32);
	let aU64 = AtomicU64::new(64);
	let aUsize = AtomicUsize::new(88);

	println!("ab: {:p}", &ab);

	println!("aI8: {:p}", &aI8);
	println!("aI16: {:p}", &aI16);
	println!("aI32: {:p}", &aI32);
	println!("aI64: {:p}", &aI64);
	println!("aIsize: {:p}", &aIsize);

	println!("aPtr: {:p}", &aPtr);

	println!("aU8: {:p}", &aU8);
	println!("aU16: {:p}", &aU16);
	println!("aU32: {:p}", &aU32);
	println!("aU64: {:p}", &aU64);
	println!("aUsize: {:p}", &aUsize);

	ab.store(ab.load(sc), sc);

	aI8.store(aI8.load(sc), sc);
	aI16.store(aI16.load(sc), sc);
	aI32.store(aI32.load(sc), sc);
	aI64.store(aI64.load(sc), sc);
	aIsize.store(aIsize.load(sc), sc);

	aPtr.store(aPtr.load(sc), sc);

	aU8.store(aU8.load(sc), sc);
	aU16.store(aU16.load(sc), sc);
	aU32.store(aU32.load(sc), sc);
	aU64.store(aU64.load(sc), sc);
	aUsize.store(aUsize.load(sc), sc);
}
