import { createRequire } from "node:module";
const wcw = createRequire(import.meta.url)("../build/Release/watcher-napi.node");

export enum PathType {
  dir = 'dir',
  file = 'file',
  hardLink = 'hardLink',
  symLink = 'symLink',
  watcher = 'watcher',
  other = 'other',
}

export enum EffectType {
  rename = 'rename',
  modify = 'modify',
  create = 'create',
  destroy = 'destroy',
  owner = 'owner',
  other = 'other',
}

export interface Event {
  pathName: string;
  effectType: EffectType;
  pathType: PathType;
  associatedPathName: string | null;
}

interface CEvent {
  pathName: string;
  effectType: number;
  pathType: number;
  associatedPathName: string | null;
}

export const watch = (path: string, cb: (event: Event) => void): { close: () => boolean } => {
  let typedCb: null | ((_: CEvent) => void) = (event) => {
    cb({
      pathName: event.pathName,
      effectType: Object.keys(EffectType)[event.effectType] as EffectType,
      pathType: Object.keys(PathType)[event.pathType] as PathType,
      associatedPathName: event.associatedPathName,
    });
  };
  let watcher = wcw.watch(path, typedCb);
  return {
    close: (): boolean => {
      if (!watcher || !typedCb) {
        return false;
      }
      const ok = watcher.close();
      watcher = null;
      typedCb = null;
      return ok;
    }
  };
}
