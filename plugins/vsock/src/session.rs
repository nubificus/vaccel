use crate::*;

use protocols::agent::{CreateSessionRequest, DestroySessionRequest};
use crate::util::vsock_client;

pub fn sess_init(sess: &mut vaccel_session, flags: u32) -> Result<u32, u32> {
    let mut req = CreateSessionRequest::default();
    req.flags = flags;

    let client = vsock_client()?;

    match client.create_session(&req, 0) {
        Err(_) => Err(VACCEL_EIO),
        Ok(resp) => {
            sess.session_id = resp.session_id;
            Ok(VACCEL_OK)
        }
    }
}

pub fn sess_free(sess: &vaccel_session) -> Result<u32, u32> {
    let mut req = DestroySessionRequest::default();
    req.session_id = sess.session_id;

    let client = vsock_client()?;

    match client.destroy_session(&req, 0) {
        Err(_) => Err(VACCEL_EIO),
        Ok(_) => Ok(VACCEL_OK)
    }
}
