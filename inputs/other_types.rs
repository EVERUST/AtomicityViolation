#![allow(warnings)]

fn main() {
	// store
	let mut integer: i32 = 1;
	let mut float: f64 = 3.14;
	let mut boolean: bool = true;
	let mut character: char = 'c';
	let mut tuple: (i32, f64, bool) = (2, 1.592, true);
	let mut arr: [i32; 5] = [1, 2, 3, 4, 5];

	// load & store
	integer = integer + 1;
	float = float + 0.4;
	boolean = !boolean;
	if(character == 'a') {
		println!("hello");
	}
	tuple.0 = tuple.0 + 2;
	arr[3] = -1 * arr[3];
}
