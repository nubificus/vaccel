use crate::resources::VaccelResource;
use crate::*;

use std::fs::File;
use std::io::Read;
use std::path::Path;

use protocols::resources::{CreateResourceRequest, CreateTensorflowModelRequest};

pub struct TensorFlowModel {
    data: Vec<u8>,
}

impl TensorFlowModel {
    pub fn new(path: &Path) -> Result<Self, u32> {
        let mut file = File::open(path).map_err(|_| VACCEL_EIO)?;

        let mut buff = Vec::new();
        file.read_to_end(&mut buff).map_err(|_| VACCEL_EIO)?;

        Ok(TensorFlowModel { data: buff })
    }

    pub fn from_vec(buf: &Vec<u8>) -> Result<Self, u32> {
        Ok(TensorFlowModel { data: buf.clone() })
    }
}

impl VaccelResource for TensorFlowModel {
    fn create_resource_request(self) -> Result<CreateResourceRequest, u32> {
        let mut model = CreateTensorflowModelRequest::new();
        model.set_model(self.data);

        let mut req = CreateResourceRequest::new();
        req.set_tf(model);

        Ok(req)
    }
}
