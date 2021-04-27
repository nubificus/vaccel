use crate::client::VsockClient;
use crate::*;
use protocols::session::{CreateSessionRequest, DestroySessionRequest};

impl VsockClient {
    pub fn sess_init(&self, flags: u32) -> Result<u32, u32> {
        let ctx = ttrpc::context::Context::default();
        let mut req = CreateSessionRequest::default();
        req.flags = flags;

        match self.ttrpc_client.create_session(ctx, &req) {
            Err(_) => Err(VACCEL_EIO),
            Ok(resp) => Ok(resp.session_id),
        }
    }

    pub fn sess_free(&self, sess_id: u32) -> Result<(), u32> {
        let ctx = ttrpc::context::Context::default();
        let mut req = DestroySessionRequest::default();
        req.session_id = sess_id;

        match self.ttrpc_client.destroy_session(ctx, &req) {
            Err(_) => Err(VACCEL_EIO),
            Ok(_) => Ok(()),
        }
    }
}
