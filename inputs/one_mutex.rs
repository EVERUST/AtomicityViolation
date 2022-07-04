//use std::thread;
use std::sync::Mutex;

fn main() {
	let data = Mutex::new(0);
	{
		let mut d = data.lock().unwrap();
		*d += 1;
		println!("d address: {:p}", &d);
	}
	let dd = data.lock().unwrap();
	println!("dd address: {:p}", &dd);
	println!("mutex address: {:p}", &data);
}



