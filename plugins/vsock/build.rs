extern crate bindgen;

use std::env;
use std::path::PathBuf;

fn main() {
    let include_path = env::var("INCLUDE_PATH").unwrap_or("/usr/local/include".to_string());

    let clang_arg = String::from(format!("-I{}", include_path));

    cc::Build::new()
        .file("src/glue.c")
        .include(include_path)
        .compile("vaccel_glue");

    // Rebuild whenever wrapper changes
    println!("cargo:rerun-if-changed=wrapper.h");
    println!("cargo:rerun-if-changed=src/glue.c");

    let bindings = bindgen::Builder::default()
        .header("wrapper.h")
        .clang_arg(clang_arg.clone())
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .derive_default(true)
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
