use futures::StreamExt;
use std::env::args;
use wtr_watcher::Watch;

#[tokio::main(flavor = "current_thread")]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let path = args().nth(1).unwrap_or_else(|| ".".to_string());
    let show = |e| async move { println!("{e:?}") };
    let events = Watch::try_new(&path)?;
    events.for_each(show).await;
    Ok(())
}
