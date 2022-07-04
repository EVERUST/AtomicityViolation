use std::sync::Mutex;

fn main() {
	let m1 = Mutex::new(0);
	let m2 = Mutex::new(1);
	let m3 = Mutex::new(2);

	{
		let _d1 = m1.lock().unwrap();
	}

	{
		let _d2 = m2.lock().unwrap();
		{
			let _d3 = m3.lock().unwrap();
		}
	}
}
