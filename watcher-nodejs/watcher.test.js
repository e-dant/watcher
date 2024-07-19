const watcher = require('./build/Debug/watcher');
const fs = require('fs').promises;
const path = require('path');

describe('Watcher', () => {
  const testDir = path.join(__dirname, 'd');
  const testFile = path.join(testDir, 'f');

  beforeAll(async () => {
    await fs.mkdir(testDir, { recursive: true });
  });

  afterAll(async () => {
    await fs.rm(testDir, { recursive: true });
  });

  test('Module is typed ok', () => {
    expect(typeof watcher.watch).toBe('function');
    const w = watcher.watch(testDir, (_) => {});
    expect(typeof w).toBe('object');
    expect(typeof w.close).toBe('function');
    w.close();
  }, 1);

  test('Detects creation', (done) => {
    const w = watcher.watch(testDir, (event) => {
      if ('self/live' in event) {
        expect('s/self/live' in event).toBe(true);
      } else {
        expect(event.pathName).toBe(testFile);
        expect(event.effectType).toBe(2); // WTR_WATCHER_EVENT_CREATE
        expect(event.pathType).toBe(1); // WTR_WATCHER_PATH_FILE
        done();
      }
    });
    setTimeout(() => { fs.writeFile(testFile, 'stuff'); }, 200);
    setTimeout(w.close, 300);
  }, 1000);

  test('Detects modification', (done) => {
    const w = watcher.watch(testDir, (event) => {
      if ('self/live' in event) {
        expect('s/self/live' in event).toBe(true);
      } else {
        expect(event.effectType).toBe(1); // WTR_WATCHER_EVENT_MODIFY
        expect(event.pathName).toBe(testFile);
        expect(event.pathType).toBe(1); // WTR_WATCHER_PATH_FILE
      }
      done();
    });
    setTimeout(() => { fs.appendFile(testFile, 'more stuff'); }, 200);
    setTimeout(w.close, 300);
  }, 1000);

  test('Detects file deletion', (done) => {
    const w = watcher.watch(testDir, (event) => {
      if ('self/live' in event) {
        expect('s/self/live' in event).toBe(true);
      } else {
        expect(event.effectType).toBe(3); // WTR_WATCHER_EVENT_DELETE
        expect(event.pathName).toBe(testFile);
        expect(event.pathType).toBe(1); // WTR_WATCHER_PATH_FILE
        done();
      }
    });
    setTimeout(() => { fs.unlink(testFile); }, 200);
    setTimeout(w.close, 300);
  }, 1000);
});
