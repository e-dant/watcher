#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
include!(concat!(env!("OUT_DIR"), "/watcher_c.rs"));
use core::ffi::c_void;
use serde::Deserialize;
use serde::Serialize;
use tokio::sync::mpsc::Receiver;
use tokio::sync::mpsc::Sender;

#[derive(Serialize, Deserialize, Debug, Clone, Copy)]
pub enum EffectType {
    Rename,
    Modify,
    Create,
    Destroy,
    Owner,
    Other,
}

#[derive(Serialize, Deserialize, Debug, Clone, Copy)]
pub enum PathType {
    Dir,
    File,
    HardLink,
    SymLink,
    Watcher,
    Other,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Event {
    pub effect_time: i64,
    pub path_name: String,
    pub associated_path_name: String,
    pub effect_type: EffectType,
    pub path_type: PathType,
}

fn c_ptr_as_str<'a>(ptr: *const std::os::raw::c_char) -> &'a str {
    if ptr.is_null() {
        return "";
    }
    let b = unsafe { std::ffi::CStr::from_ptr(ptr).to_bytes() };
    std::str::from_utf8(b).unwrap_or_default()
}

fn effect_type_from_c(effect_type: i8) -> EffectType {
    match effect_type {
        WTR_WATCHER_EFFECT_RENAME => EffectType::Rename,
        WTR_WATCHER_EFFECT_MODIFY => EffectType::Modify,
        WTR_WATCHER_EFFECT_CREATE => EffectType::Create,
        WTR_WATCHER_EFFECT_DESTROY => EffectType::Destroy,
        WTR_WATCHER_EFFECT_OWNER => EffectType::Owner,
        WTR_WATCHER_EFFECT_OTHER => EffectType::Other,
        _ => EffectType::Other,
    }
}

fn path_type_from_c(path_type: i8) -> PathType {
    match path_type {
        WTR_WATCHER_PATH_DIR => PathType::Dir,
        WTR_WATCHER_PATH_FILE => PathType::File,
        WTR_WATCHER_PATH_HARD_LINK => PathType::HardLink,
        WTR_WATCHER_PATH_SYM_LINK => PathType::SymLink,
        WTR_WATCHER_PATH_WATCHER => PathType::Watcher,
        WTR_WATCHER_PATH_OTHER => PathType::Other,
        _ => PathType::Other,
    }
}

fn ev_from_c<'a>(event: wtr_watcher_event) -> Event {
    Event {
        effect_time: event.effect_time,
        path_name: c_ptr_as_str(event.path_name).to_string(),
        associated_path_name: c_ptr_as_str(event.associated_path_name).to_string(),
        effect_type: effect_type_from_c(event.effect_type),
        path_type: path_type_from_c(event.path_type),
    }
}

unsafe extern "C" fn callback_bridge(event: wtr_watcher_event, data: *mut c_void) {
    let ev = ev_from_c(event);
    let tx = std::mem::transmute::<*mut c_void, &Sender<Event>>(data);
    let _ = tx.blocking_send(ev);
}

#[allow(dead_code)]
pub struct Watch {
    watcher: *mut c_void,
    ev_rx: Receiver<Event>,
    ev_tx: Box<Sender<Event>>,
}

impl Watch {
    pub fn try_new(path: &str) -> Result<Watch, &'static str> {
        let path = std::ffi::CString::new(path).unwrap();
        let (ev_tx, ev_rx) = tokio::sync::mpsc::channel(1);
        let ev_tx = Box::new(ev_tx);
        let ev_tx_opaque = unsafe { std::mem::transmute::<&Sender<Event>, *mut c_void>(&ev_tx) };
        match unsafe { wtr_watcher_open(path.as_ptr(), Some(callback_bridge), ev_tx_opaque) } {
            watcher if watcher.is_null() => Err("wtr_watcher_open"),
            watcher => Ok(Watch {
                watcher,
                ev_rx,
                ev_tx,
            }),
        }
        /*
        let watcher_opaque =
            unsafe { wtr_watcher_open(path.as_ptr(), Some(callback_bridge), ev_tx_opaque) };
        if watcher_opaque.is_null() {
            Err("wtr_watcher_open")
        } else {
            Ok(Watcher {
                watcher_opaque,
                ev_rx,
                ev_tx,
            })
        }
        */
    }

    pub fn close(&self) -> Result<(), &'static str> {
        match unsafe { wtr_watcher_close(self.watcher) } {
            false => Err("wtr_watcher_close"),
            true => Ok(()),
        }
    }
}

impl futures::Stream for Watch {
    type Item = Event;

    fn poll_next(
        mut self: std::pin::Pin<&mut Self>,
        cx: &mut std::task::Context,
    ) -> std::task::Poll<Option<Self::Item>> {
        self.ev_rx.poll_recv(cx)
    }
}

impl Drop for Watch {
    fn drop(&mut self) {
        let _ = self.close();
    }
}
