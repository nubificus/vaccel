#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(dead_code)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

use std::os::raw::c_uchar;
use std::slice;

pub mod vaccel;
pub mod session;
pub mod resources;
mod util;

#[no_mangle]
pub unsafe extern "C"
fn sess_init(sess: &mut vaccel_session, flags: u32) -> i32 {
    match session::sess_init(&mut (*sess), flags) {
        Ok(ret) => ret as i32,
        Err(ret) => ret as i32,
    }
}

#[no_mangle]
pub extern "C"
fn sess_free(sess: &vaccel_session) -> i32 {
    match session::sess_free(sess) {
        Ok(ret) => ret as i32,
        Err(ret) => ret as i32,
    }
}

#[no_mangle]
pub extern "C"
fn image_classify(
    sess: &vaccel_session,
    img: *const c_uchar,
    img_len: usize,
    tags: *mut c_uchar,
    tags_len: usize,
) -> i32 {

    let img = unsafe { slice::from_raw_parts(img, img_len) };
    let tags_slice = unsafe { slice::from_raw_parts_mut(tags, tags_len) };

    match vaccel::image_classify(sess, img.to_vec()) {
        Err(ret) => ret as i32,
        Ok(ret) => {
            tags_slice.copy_from_slice(&ret[..tags_slice.len()]);
            std::mem::forget(tags_slice);

            VACCEL_OK as i32
        }
    }
}
