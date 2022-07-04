use std::sync::{Arc, RwLock, mpsc} ;
use std::sync::mpsc::{SendError, TrySendError} ;
use std::thread ;
use std::time::Duration ;

unsafe impl Send for Peers {} 
unsafe impl Sync for Peers {} 

pub struct Peers {
	peers: RwLock<Vec<Arc<RwLock<Peer>>>>,
}

pub struct Peer {
	send_channel: Option<mpsc::SyncSender<u32>>,
	close_channel: Option<mpsc::Sender<()>>,
}

impl Peers {
	pub fn new () -> Peers {
		Peers {
			peers: RwLock::new(Vec::new()),
		}
	}

	pub fn add_peer(&self) {
		let new_peer = Peer::new() ;
		
		let	insert_peer = Arc::new(RwLock::new(new_peer)) ;
		{
			let mut peers = self.peers.write().unwrap() ;
			peers.push(insert_peer.clone()) ;
		}

		thread::sleep(Duration::from_secs(5)) ;

		{
			let mut start_peer = insert_peer.write().unwrap();
			start_peer.start() ;
		}
	}

	pub fn broadcast(&self, val: u32) {
		let peers = self.peers.write().unwrap() ;
		for peer in peers.iter() {
			let peer = peer.read().unwrap() ;
			if let Err(_) = peer.send(val) {
				println!("broadcast err!") ;
			}
		}
	}

	pub fn close_peers(&self) {
		let mut peers = self.peers.write().unwrap() ;
		for peer in peers.drain(..) {
			let peer = peer.read().unwrap() ;
			if let Err(_) = peer.close() {
				println!("close_peers err!") ;
			}
		}
	}
		
}

impl Peer {
	pub fn new() -> Peer {
		Peer {
			send_channel: None,
			close_channel: None,
		}
	}

	pub fn send(&self, val: u32) -> Result<(), TrySendError<u32>> {
		self.send_channel.as_ref().unwrap().try_send(val)? ;	
		Ok(())
	}

	pub fn start(&mut self) {
		let (send_channel, close_channel) = listen() ;
		self.send_channel = Some(send_channel) ;
		self.close_channel = Some(close_channel) ;
	}

	pub fn close(&self) -> Result<(), SendError<()>> {
		self.close_channel.as_ref().unwrap().send(())? ;
		Ok(())
	}
}

pub fn listen() -> (mpsc::SyncSender<u32>, mpsc::Sender<()>) {
	let (send_tx, send_rx) = mpsc::sync_channel(10) ;
	let (close_tx, close_rx) = mpsc::channel() ;
	
	poll(send_rx, close_rx) ;
	
	(send_tx, close_tx)
}

fn poll(send_rx: mpsc::Receiver<u32>, close_rx: mpsc::Receiver<()>) {
	let _ =	thread::spawn(move || {
		loop {
			thread::sleep(Duration::from_secs(3)) ;
			if let Ok(val) = send_rx.try_recv() {
				println!("recv: {}", val) ;
			}

			if let Ok(_) = close_rx.try_recv() {
				println!("closing...") ;
				break ;
			}
		}
		thread::sleep(Duration::from_millis(1)) ;
	}) ;
}

fn main() {
	let peers = Arc::new(Peers::new()) ;

	let peers_clone = peers.clone() ;
	thread::spawn(move || {
			peers_clone.add_peer() ;
	}) ;

	thread::sleep(Duration::from_secs(2)) ;
	peers.close_peers() ;

	thread::sleep(Duration::from_secs(3)) ;
}

