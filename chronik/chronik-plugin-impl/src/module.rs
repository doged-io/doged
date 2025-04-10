// Copyright (c) 2024 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//! Module for Python module related things

use std::ffi::CString;

use pyo3::{prelude::*, types::PyList};

/// Python module for Chronik plugins
#[pymodule]
pub fn chronik_plugin(
    py: Python<'_>,
    _module: Bound<'_, PyModule>,
) -> PyResult<()> {
    PyModule::from_code(
        py,
        CString::new(include_str!("plugin.py"))?.as_c_str(),
        CString::new("plugin.py")?.as_c_str(),
        CString::new("chronik_plugin.plugin")?.as_c_str(),
    )?;
    // Re-use `script.py` from the test framework
    PyModule::from_code(
        py,
        CString::new(include_str!(
            "../../../test/functional/test_framework/script.py"
        ))?
        .as_c_str(),
        CString::new("script.py")?.as_c_str(),
        CString::new("chronik_plugin.script")?.as_c_str(),
    )?;
    // Re-use `slp.py` from the test framework, need to patch the import
    PyModule::from_code(
        py,
        CString::new(
            include_str!(
                "../../../test/functional/test_framework/chronik/slp.py"
            )
            .replace("test_framework", "chronik_plugin"),
        )?
        .as_c_str(),
        CString::new("slp.py")?.as_c_str(),
        CString::new("chronik_plugin.slp")?.as_c_str(),
    )?;
    PyModule::from_code(
        py,
        CString::new(include_str!("etoken.py"))?.as_c_str(),
        CString::new("etoken.py")?.as_c_str(),
        CString::new("chronik_plugin.etoken")?.as_c_str(),
    )?;
    PyModule::from_code(
        py,
        CString::new(include_str!("tx.py"))?.as_c_str(),
        CString::new("tx.py")?.as_c_str(),
        CString::new("chronik_plugin.tx")?.as_c_str(),
    )?;
    Ok(())
}

/// Load the `chronik_plugin` Python module
pub fn load_plugin_module() {
    pyo3::append_to_inittab!(chronik_plugin);
}

/// Add functional test folder to PYTHONPATH so we can use its utils
pub fn add_test_framework_to_pythonpath(py: Python<'_>) -> PyResult<()> {
    let sys = PyModule::import(py, "sys")?;
    let path = sys.getattr("path")?;
    let sys_path = path.downcast::<PyList>()?;
    let functional_path =
        concat!(env!("CARGO_MANIFEST_DIR"), "/../../test/functional");

    if !sys_path.contains(functional_path)? {
        sys_path.append(functional_path)?;
    }

    Ok(())
}
