#![allow(unused)]

use std::fmt;
use std::ops;

pub struct AtomicUsize {
    inner: std::cell::UnsafeCell<std::sync::atomic::AtomicUsize>,
}

unsafe impl Send for AtomicUsize {}
unsafe impl Sync for AtomicUsize {}

impl AtomicUsize {
    pub const fn new(val: usize) -> AtomicUsize {
        let inner = std::cell::UnsafeCell::new(std::sync::atomic::AtomicUsize::new(val));
        AtomicUsize { inner }
    }

    pub unsafe fn unsync_load(&self) -> usize {
        *(*self.inner.get()).get_mut()
    }

    pub fn with_mut<R>(&mut self, f: impl FnOnce(&mut usize) -> R) -> R {
        // safety: we have mutable access
        f(unsafe { (*self.inner.get()).get_mut() })
    }
}

impl ops::Deref for AtomicUsize {
    type Target = std::sync::atomic::AtomicUsize;

    fn deref(&self) -> &Self::Target {
        // safety: it is always safe to access `&self` fns on the inner value as
        // we never perform unsafe mutations.
        unsafe { &*self.inner.get() }
    }
}

impl ops::DerefMut for AtomicUsize {
    fn deref_mut(&mut self) -> &mut Self::Target {
        // safety: we hold `&mut self`
        unsafe { &mut *self.inner.get() }
    }
}

impl fmt::Debug for AtomicUsize {
    fn fmt(&self, fmt: &mut fmt::Formatter<'_>) -> fmt::Result {
        (**self).fmt(fmt)
    }
}


// -------------------------


#[derive(Debug)]
pub struct UnsafeCell<T>(std::cell::UnsafeCell<T>);

impl<T> UnsafeCell<T> {
    pub const fn new(data: T) -> UnsafeCell<T> {
        UnsafeCell(std::cell::UnsafeCell::new(data))
    }

    pub fn with<R>(&self, f: impl FnOnce(*const T) -> R) -> R {
        f(self.0.get())
    }

    pub fn with_mut<R>(&self, f: impl FnOnce(*mut T) -> R) -> R {
        f(self.0.get())
    }

    pub fn get(self) -> T {
	    self.0.into_inner()
    }
    
}
