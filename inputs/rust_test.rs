fn main() {
	let mut n=0;
	let mut s=0;
	println!("n: {:p}", &n);
	println!("s: {:p}", &s);
	s += n;

	n=1;
	s+=n;

	n=2;
	s+=n;

	n=3;
	s+=n;

	println!("> {}", s);
}
