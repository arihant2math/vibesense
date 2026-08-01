use std::{env, error::Error, net::SocketAddr, sync::Arc};

use tokio::net::TcpListener;
use vibesense_server::{OnnxDetector, app};

const DEFAULT_MODEL_DIR: &str = "detector-onnx";
const DEFAULT_HOST: &str = "0.0.0.0";
const DEFAULT_PORT: &str = "5000";

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    let model_dir = env::var("VIBESENSE_MODEL_DIR").unwrap_or_else(|_| DEFAULT_MODEL_DIR.into());
    eprintln!("loading Vibesense detector from {model_dir}");
    let detector = Arc::new(OnnxDetector::from_dir(&model_dir)?);

    let address: SocketAddr = bind_address().parse()?;
    let listener = TcpListener::bind(address).await?;
    eprintln!("Vibesense API listening on http://{address}");

    axum::serve(listener, app(detector))
        .with_graceful_shutdown(shutdown_signal())
        .await?;
    Ok(())
}

fn bind_address() -> String {
    if let Ok(address) = env::var("VIBESENSE_BIND") {
        return address;
    }
    let host = env::var("VIBESENSE_HOST")
        .or_else(|_| env::var("HOST"))
        .unwrap_or_else(|_| DEFAULT_HOST.into());
    let port = env::var("VIBESENSE_PORT")
        .or_else(|_| env::var("PORT"))
        .unwrap_or_else(|_| DEFAULT_PORT.into());
    format!("{host}:{port}")
}

async fn shutdown_signal() {
    if let Err(error) = tokio::signal::ctrl_c().await {
        eprintln!("could not listen for shutdown signal: {error}");
    }
}
