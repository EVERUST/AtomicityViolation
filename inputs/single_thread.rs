use std::sync::{Mutex, Arc};

fn main() {
	let m1 = Arc::new(Mutex::new(0));
	let m2 = m1.clone();

	{
		let d2 = m2.lock().unwrap();
	}

	m1.lock().unwrap();
}
