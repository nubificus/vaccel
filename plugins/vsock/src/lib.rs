#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(dead_code)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

use libc::c_void;
use std::ffi::CStr;
use std::os::raw::c_uchar;
use std::path::Path;
use std::slice;

use client::VsockClient;
use tf_model::TensorFlowModel;

pub mod client;
mod image;
mod resources;
mod session;
mod tf_model;
mod util;

#[no_mangle]
pub extern "C" fn create_client() -> *mut VsockClient {
    match VsockClient::new() {
        Ok(c) => Box::into_raw(Box::new(c)),
        Err(_) => std::ptr::null_mut(),
    }
}

#[no_mangle]
pub extern "C" fn destroy_client(client: *mut VsockClient) {
    if !client.is_null() {
        unsafe { Box::from_raw(client) };
    }
}

#[no_mangle]
pub extern "C" fn sess_init(client_ptr: *mut VsockClient, flags: u32) -> i32 {
    let client = match unsafe { client_ptr.as_ref() } {
        Some(client) => client,
        None => return VACCEL_EINVAL as i32,
    };

    match client.sess_init(flags) {
        Ok(ret) => ret as i32,
        Err(err) => -(err as i32),
    }
}

#[no_mangle]
pub extern "C" fn sess_free(client_ptr: *const VsockClient, sess_id: u32) -> i32 {
    let client = match unsafe { client_ptr.as_ref() } {
        Some(client) => client,
        None => return VACCEL_EINVAL as i32,
    };

    match client.sess_free(sess_id) {
        Ok(()) => VACCEL_OK as i32,
        Err(ret) => ret as i32,
    }
}

#[no_mangle]
pub extern "C" fn image_classify(
    client_ptr: *const VsockClient,
    sess_id: u32,
    img: *const c_uchar,
    img_len: usize,
    tags: *mut c_uchar,
    tags_len: usize,
) -> i32 {
    let img = unsafe { slice::from_raw_parts(img, img_len) };
    let tags_slice = unsafe { slice::from_raw_parts_mut(tags, tags_len) };

    let client = match unsafe { client_ptr.as_ref() } {
        Some(client) => client,
        None => return VACCEL_EINVAL as i32,
    };

    match client.image_classify(sess_id, img.to_vec()) {
        Err(ret) => ret as i32,
        Ok(ret) => {
            tags_slice.copy_from_slice(&ret[..tags_slice.len()]);
            std::mem::forget(tags_slice);

            VACCEL_OK as i32
        }
    }
}

fn create_tf_model(client: &VsockClient, model: &vaccel_tf_model) -> vaccel_id_t {
    if !model.path.is_null() {
        let cstr: &CStr = unsafe { CStr::from_ptr(model.path) };
        let rstr = match cstr.to_str() {
            Ok(rstr) => rstr,
            Err(_) => return -(VACCEL_ENOENT as i64),
        };

        let tf_model = match TensorFlowModel::new(Path::new(rstr)) {
            Ok(m) => m,
            Err(err) => return -(err as i64),
        };

        match client.create_resource(tf_model) {
            Ok(id) => id,
            Err(err) => return -(err as i64),
        }
    } else {
        let data =
            unsafe { Vec::from_raw_parts(model.data, model.size as usize, model.size as usize) };
        let tf_model = match TensorFlowModel::from_vec(&data) {
            Ok(m) => m,
            Err(err) => return -(err as i64),
        };
        std::mem::forget(data);

        match client.create_resource(tf_model) {
            Err(ret) => -(ret as i64),
            Ok(id) => id,
        }
    }
}

#[no_mangle]
pub extern "C" fn create_resource(
    client_ptr: *const VsockClient,
    res_type: vaccel_resource_t,
    data: *mut c_void,
) -> vaccel_id_t {
    if data.is_null() {
        return VACCEL_EINVAL as i64;
    }

    let client = match unsafe { client_ptr.as_ref() } {
        Some(client) => client,
        None => return VACCEL_EINVAL as i64,
    };

    match res_type {
        vaccel_resource_t_VACCEL_RES_TF_MODEL => {
            let model_ptr = data as *mut vaccel_tf_model;
            let model = unsafe { model_ptr.as_ref().unwrap() };
            create_tf_model(client, model)
        }
        _ => -(VACCEL_ENOTSUP as i64),
    }
}

#[no_mangle]
pub extern "C" fn destroy_resource(client_ptr: *const VsockClient, id: vaccel_id_t) -> i32 {
    let client = match unsafe { client_ptr.as_ref() } {
        Some(client) => client,
        None => return VACCEL_EINVAL as i32,
    };

    match client.destroy_resource(id) {
        Err(ret) => ret as i32,
        Ok(()) => VACCEL_OK as i32,
    }
}

#[no_mangle]
pub extern "C" fn register_resource(
    client_ptr: *const VsockClient,
    res_id: vaccel_id_t,
    sess_id: u32,
) -> i32 {
    let client = match unsafe { client_ptr.as_ref() } {
        Some(client) => client,
        None => return VACCEL_EINVAL as i32,
    };

    match client.register_resource(res_id, sess_id) {
        Err(ret) => ret as i32,
        Ok(()) => VACCEL_OK as i32,
    }
}

#[no_mangle]
pub extern "C" fn unregister_resource(
    client_ptr: *const VsockClient,
    res_id: vaccel_id_t,
    sess_id: u32,
) -> i32 {
    let client = match unsafe { client_ptr.as_ref() } {
        Some(client) => client,
        None => return VACCEL_EINVAL as i32,
    };

    match client.unregister_resource(res_id, sess_id) {
        Err(ret) => ret as i32,
        Ok(()) => VACCEL_OK as i32,
    }
}
