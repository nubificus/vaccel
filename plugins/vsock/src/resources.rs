use crate::client::VsockClient;
use crate::*;
use protocols::resources::{
    CreateResourceRequest, DestroyResourceRequest, RegisterResourceRequest,
    UnregisterResourceRequest,
};

pub trait VaccelResource {
    fn create_resource_request(self) -> Result<CreateResourceRequest, u32>;
}

impl VsockClient {
    pub fn create_resource(&self, resource: impl VaccelResource) -> Result<vaccel_id_t, u32> {
        let ctx = ttrpc::context::Context::default();
        let req = resource.create_resource_request()?;

        match self.ttrpc_client.create_resource(ctx, &req) {
            Err(_) => Err(VACCEL_EIO),
            Ok(resp) => Ok(resp.get_resource_id()),
        }
    }

    pub fn destroy_resource(&self, model_id: i64) -> Result<(), u32> {
        let ctx = ttrpc::context::Context::default();
        let mut req = DestroyResourceRequest::new();
        req.set_resource_id(model_id);

        self.ttrpc_client
            .destroy_resource(ctx, &req)
            .map_err(|_| VACCEL_EIO)?;

        Ok(())
    }

    pub fn register_resource(&self, model_id: i64, sess_id: u32) -> Result<(), u32> {
        let ctx = ttrpc::context::Context::default();
        let mut req = RegisterResourceRequest::new();
        req.set_resource_id(model_id);
        req.set_session_id(sess_id);

        self.ttrpc_client
            .register_resource(ctx, &req)
            .map_err(|_| VACCEL_EIO)?;

        Ok(())
    }

    pub fn unregister_resource(&self, model_id: i64, sess_id: u32) -> Result<(), u32> {
        let ctx = ttrpc::context::Context::default();
        let mut req = UnregisterResourceRequest::new();
        req.set_resource_id(model_id);
        req.set_session_id(sess_id);

        self.ttrpc_client
            .unregister_resource(ctx, &req)
            .map_err(|_| VACCEL_EIO)?;

        Ok(())
    }
}
