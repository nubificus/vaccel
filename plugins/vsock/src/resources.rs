use std::fs;
use std::ffi::{CStr, CString};

use crate::*;

use protocols::agent::{
    RegisterResourceRequest,
    UnregisterResourceRequest,
    RegisterCaffeModelRequest,
};
use crate::util::vsock_client;

fn get_string_from_c(ptr: *const std::os::raw::c_char) -> Result<String, u32> {
    let c_str: &CStr = unsafe { CStr::from_ptr(ptr) };
    let c_str =  match c_str.to_str() {
        Ok(c_str) => c_str,
        Err(_) => return Err(VACCEL_EINVAL),
    };

    let c_string = match CString::new(c_str) {
        Ok(string) => string,
        Err(_) => return Err(VACCEL_EINVAL),
    };

    match c_string.into_string() {
        Ok(s) => Ok(s),
        Err(_) => Err(VACCEL_EINVAL),
    }
}

fn read_file(path: &String) -> Result<Vec<u8>, u32> {
    match fs::read(path) {
        Ok(data) => Ok(data),
        Err(_) => Err(VACCEL_EINVAL)
    }
}

pub fn register_ml_model(
    sess: &vaccel_session,
    model: &mut vaccel_ml_caffe_model
) -> Result<u32, u32> {
    let mut req = RegisterResourceRequest::default();
    let mut caffe = RegisterCaffeModelRequest::default();

    caffe.session_id = sess.session_id;
    caffe.prototxt = read_file(&get_string_from_c(model.prototxt)?)?;
    caffe.binary_model = read_file(&get_string_from_c(model.bin_model)?)?;
    caffe.labels = read_file(&get_string_from_c(model.labels)?)?;

    req.set_caffe(caffe);

    let client = vsock_client()?;

    match client.register_resource(&req, 0) {
        Err(_) => Err(VACCEL_EIO),
        Ok(resp) => {
            model.ml_model.resource.id = resp.resource_id;
            Ok(VACCEL_OK)
        }
    }
}

pub fn unregister_resource(
    sess: &vaccel_session,
    resource: &vaccel_resource
) -> Result<u32, u32> {
    let mut req = UnregisterResourceRequest::default();
    req.session_id = sess.session_id;
    req.resource_id = resource.id;

    let client = vsock_client()?;

    match client.unregister_resource(&req, 0) {
        Err(_) => Err(VACCEL_EIO),
        Ok(_) => Ok(VACCEL_OK)
    }
}
