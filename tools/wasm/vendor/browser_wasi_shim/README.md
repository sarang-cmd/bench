# browser_wasi_shim, vendored

`@bjorn3/browser_wasi_shim` 0.4.2, unmodified `dist/` files, MIT OR Apache-2.0
(both licences are next to this file).

The WebAssembly build is a WASI reactor, so the page needs a WASI
implementation. Vendored rather than installed from npm so the site has no
runtime dependency on a package registry: `site/scripts/copy-wasm.mjs` copies
these files next to `bench.wasm` and the worker imports them relatively.

Upstream: <https://github.com/bjorn3/browser_wasi_shim>
