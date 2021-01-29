use crate::*;

use protocols::agent::ImageClassificationRequest;
use crate::util::vsock_client;

pub fn image_classify(
    sess: &vaccel_session,
    img: Vec<u8>
) -> Result<Vec<u8>, u32> {
    let mut req = ImageClassificationRequest::default();
    req.set_session_id(sess.session_id);
    req.set_image(img);

    let client = vsock_client()?;

    match client.image_classification(&req, 0) {
        Err(_) => Err(VACCEL_EIO),
        Ok(resp) => Ok(resp.tags)
    }
}
