#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

use nix::sys::socket::{connect, socket, AddressFamily, SockAddr, SockFlag, SockType};
use std::env;
use std::os::unix::io::RawFd;
use ttrpc;

fn client_create_vsock_fd(cid: libc::c_uint, port: u32) -> Result<RawFd, u32> {
    let fd = socket(
        AddressFamily::Vsock,
        SockType::Stream,
        SockFlag::SOCK_CLOEXEC,
        None,
    )
    .map_err(|_| VACCEL_EIO)?;

    let sock_addr = SockAddr::new_vsock(cid, port);

    connect(fd, &sock_addr).map_err(|_| VACCEL_EIO)?;

    Ok(fd)
}

pub fn create_ttrpc_client(server_address: &String) -> Result<ttrpc::Client, u32> {
    if server_address == "" {
        return Err(VACCEL_EINVAL);
    }

    let fields: Vec<&str> = server_address.split("://").collect();

    if fields.len() != 2 {
        return Err(VACCEL_EINVAL);
    }

    let scheme = fields[0].to_lowercase();

    let fd: RawFd = match scheme.as_str() {
        "vsock" => {
            let addr: Vec<&str> = fields[1].split(':').collect();

            if addr.len() != 2 {
                return Err(VACCEL_EINVAL);
            }

            let cid: u32 = match addr[0] {
                "-1" | "" => libc::VMADDR_CID_ANY,
                _ => match addr[0].parse::<u32>() {
                    Ok(c) => c,
                    Err(_) => return Err(VACCEL_EINVAL),
                },
            };

            let port: u32 = match addr[1].parse::<u32>() {
                Ok(p) => p,
                Err(_) => return Err(VACCEL_EINVAL),
            };

            client_create_vsock_fd(cid, port).map_err(|_| VACCEL_EINVAL)?
        }
        _ => return Err(VACCEL_ENOTSUP),
    };

    Ok(ttrpc::client::Client::new(fd))
}
