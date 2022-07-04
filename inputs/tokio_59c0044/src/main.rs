#![allow(unused)]

use tokio_59c0044::{AtomicUsize, UnsafeCell};
use std::sync::Arc;
use std::sync::atomic::Ordering::{self, Acquire, AcqRel};
use std::time::Duration;
use std::thread;

fn main() {
	let inner = Arc::new(Inner {
		state: AtomicUsize::new(State::new().as_usize()),
		value: UnsafeCell::new(None),
	});

	inner.close();

	let tx = Sender {
		inner: Some(inner.clone()),
	};
	let mut rx = Receiver { inner: Some(inner) } ;

	let send_handler = thread::spawn(move || {
		match tx.send(UnsafeCell::new(Some(3))) {
			Err(_) => println!("send err"),
			Ok(_) => println!("send ok"),
		};
	});

	let recv_handler = thread::spawn(move || {
		loop {
			match rx.try_recv() {
				Err(str) => {
					println!("try_recv err - {}", str);
					if str.eq("inner is not referencable") {
						break;
					}
				},
				Ok(got) => {
					println!("try_recv ok - {:?}", got.get());
					break;
				},
			};
		};
	});

	send_handler.join();
	recv_handler.join();

}

const VALUE_SENT: usize = 0b00010;
const CLOSED: usize = 0b00100;

pub struct Sender<T> {
	inner: Option<Arc<Inner<T>>>,
}

pub struct Receiver<T> {
	inner: Option<Arc<Inner<T>>>,
}

struct Inner<T> {
	state: AtomicUsize,
	value: UnsafeCell<Option<T>>,
}

#[derive(Clone, Copy)]
struct State(usize);

impl<T> Sender<T> {
	fn send(mut self, t: T) -> Result<(), T>  {
		let inner = self.inner.take().unwrap();

		inner.value.with_mut(|ptr| unsafe {
			*ptr = Some(t);
		});


		if !inner.complete() {
			thread::sleep(Duration::from_secs(2));
			unsafe {
				return Err(inner.consume_value().unwrap());
			}
		}
		Ok(())
	}
}

impl<T> Drop for Sender<T> {
	fn drop(&mut self) {
		if let Some(inner) = self.inner.as_ref() {
			inner.complete();
		}
	}
}

impl<T> Receiver<T> {
	fn try_recv(&mut self) -> Result<T, String> {
		let result = if let Some(inner) = self.inner.as_ref() {
			let state = State::load(&inner.state, Acquire);

			if state.is_complete() {
				match unsafe { inner.consume_value() } {
					Some(value) => Ok(value),
					None => Err(String::from("receiver consume value failed!")),
				}
			} else if state.is_closed() {
				Err(String::from("channel is closed!"))
			}
			/*
			if state.is_closed() {
				Err(String::from("channel is closed!"))
			} else if state.is_complete() {
				match unsafe { inner.consume_value() } {
					Some(value) => Ok(value),
					None => Err(String::from("receiver consume value failed!")),
				}
			}
			*/
			else {
				Err(String::from("not complete not closed"))
			}
		} else {
			Err(String::from("inner is not referencable"))
		};

		self.inner = None;
		result
	}
}

impl<T> Drop for Receiver<T> {
	fn drop(&mut self) {
		if let Some(inner) = self.inner.as_ref() {
			inner.close();
		}
	}
}

impl<T> Inner<T> {

	fn complete(&self) -> bool {
		let prev = State::set_complete(&self.state);

		if prev.is_closed() {
			return false;
		}

		true
	}

	fn consume_value(&self) -> Option<T> {
		self.value.with_mut(|ptr| unsafe {
			(*ptr).take()
		})
	}

	fn close(&self) {
		let prev = State::set_closed(&self.state);
	}
}

unsafe impl<T: Send> Send for Inner<T> {}
unsafe impl<T: Send> Sync for Inner<T> {}

impl State {

	fn new() -> State {
		State(0)
	}

	fn is_complete(self) -> bool {
		self.0 & VALUE_SENT == VALUE_SENT
	}

	fn is_closed(self) -> bool {
		self.0 & CLOSED == CLOSED
	}

	fn set_complete(cell: &AtomicUsize) -> State{
		let val = cell.fetch_or(VALUE_SENT, AcqRel);
		State(val)
	}

	fn set_closed(cell: &AtomicUsize) -> State {
		let val = cell.fetch_or(CLOSED, Acquire);
		State(val)
	}

	fn load(cell: &AtomicUsize, order: Ordering) -> State {
		let val = cell.load(order);
		State(val)
	}

	fn as_usize(self) -> usize {
		self.0
	}
	
}

pub fn channel<T>() -> (Sender<T>, Receiver<T>) {
	let inner = Arc::new(Inner {
		state: AtomicUsize::new(State::new().as_usize()),
		value: UnsafeCell::new(None),
	});

	let tx = Sender {
		inner: Some(inner.clone()),
	};
	let rx = Receiver { inner: Some(inner) } ;

	(tx, rx)
}

