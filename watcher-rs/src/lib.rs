#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
include!(concat!(env!("OUT_DIR"), "/watcher_c.rs"));
use core::ffi::c_char;
use core::ffi::c_void;
use core::ffi::CStr;
use core::mem::transmute;
use core::pin::Pin;
use core::str;
use core::task::Context;
use core::task::Poll;
use futures::Stream;
use serde::Deserialize;
use serde::Serialize;
use std::ffi::CString;
use tokio::sync::mpsc::channel as async_channel;
use tokio::sync::mpsc::Receiver;
use tokio::sync::mpsc::Sender;

#[derive(Serialize, Deserialize, Debug, Clone, Copy)]
#[serde(rename_all = "snake_case")]
pub enum EffectType {
    Rename,
    Modify,
    Create,
    Destroy,
    Owner,
    Other,
}

#[derive(Serialize, Deserialize, Debug, Clone, Copy)]
#[serde(rename_all = "snake_case")]
pub enum PathType {
    Dir,
    File,
    HardLink,
    SymLink,
    Watcher,
    Other,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(rename = "event")]
pub struct Event {
    pub effect_time: i64,
    pub path_name: String,
    pub associated_path_name: Option<String>,
    pub effect_type: EffectType,
    pub path_type: PathType,
}

fn c_chars_as_str<'a>(ptr: *const c_char) -> Option<&'a str> {
    match ptr.is_null() {
        true => None,
        false => {
            let b = unsafe { CStr::from_ptr(ptr).to_bytes() };
            let s = str::from_utf8(b).unwrap_or_default();
            Some(s)
        }
    }
}

fn effect_type_from_c(effect_type: i8) -> EffectType {
    use EffectType::*;
    match effect_type {
        WTR_WATCHER_EFFECT_RENAME => Rename,
        WTR_WATCHER_EFFECT_MODIFY => Modify,
        WTR_WATCHER_EFFECT_CREATE => Create,
        WTR_WATCHER_EFFECT_DESTROY => Destroy,
        WTR_WATCHER_EFFECT_OWNER => Owner,
        WTR_WATCHER_EFFECT_OTHER => Other,
        _ => Other,
    }
}

fn path_type_from_c(path_type: i8) -> PathType {
    use PathType::*;
    match path_type {
        WTR_WATCHER_PATH_DIR => Dir,
        WTR_WATCHER_PATH_FILE => File,
        WTR_WATCHER_PATH_HARD_LINK => HardLink,
        WTR_WATCHER_PATH_SYM_LINK => SymLink,
        WTR_WATCHER_PATH_WATCHER => Watcher,
        WTR_WATCHER_PATH_OTHER => Other,
        _ => Other,
    }
}

fn ev_from_c<'a>(ev: wtr_watcher_event) -> Event {
    Event {
        effect_time: ev.effect_time,
        path_name: c_chars_as_str(ev.path_name).unwrap_or_default().to_string(),
        associated_path_name: c_chars_as_str(ev.associated_path_name).map(str::to_string),
        effect_type: effect_type_from_c(ev.effect_type),
        path_type: path_type_from_c(ev.path_type),
    }
}

unsafe extern "C" fn callback_bridge(ev: wtr_watcher_event, cx: *mut c_void) {
    let ev = ev_from_c(ev);
    let tx = transmute::<*mut c_void, &Sender<Event>>(cx);
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
        let path = CString::new(path).unwrap();
        let (ev_tx, ev_rx) = async_channel(1);
        let ev_tx = Box::new(ev_tx);
        let ev_tx_opaque = unsafe { transmute::<&Sender<Event>, *mut c_void>(&ev_tx) };
        let watcher =
            unsafe { wtr_watcher_open(path.as_ptr(), Some(callback_bridge), ev_tx_opaque) };
        if watcher.is_null() {
            Err("wtr_watcher_open")
        } else {
            Ok(Watch {
                watcher,
                ev_rx,
                ev_tx,
            })
        }
    }

    pub fn close(&self) -> Result<(), &'static str> {
        match unsafe { wtr_watcher_close(self.watcher) } {
            false => Err("wtr_watcher_close"),
            true => Ok(()),
        }
    }
}

impl Stream for Watch {
    type Item = Event;

    fn poll_next(mut self: Pin<&mut Self>, cx: &mut Context) -> Poll<Option<Event>> {
        self.ev_rx.poll_recv(cx)
    }
}

impl Drop for Watch {
    fn drop(&mut self) {
        let _ = self.close();
    }
}
