package watcher_test

import (
	"fmt"
	"log/slog"
	"os"
	"path/filepath"
	"strings"
	"testing"
	"testing/synctest"

	"github.com/e-dant/watcher-go"
)

func Example() {
	w := watcher.NewWatcher("/path/to/dir", func(e *watcher.Event) {
		slog.Info("filesystem event", "event", e)
	})
	defer w.Close()

	// Wait for a new line to exit
	_, _ = fmt.Scanln()
}

func TestWatcher(t *testing.T) {
	dir := t.TempDir()
	fileEvents := make(chan *watcher.Event)
	ready := make(chan struct{})

	synctest.Test(t, func(t *testing.T) {
		go func() {
			e := <-fileEvents
			if e.EffectType != watcher.EffectTypeCreate {
				t.Errorf("expected create event, got %v", e.EffectType)
			}

			e = <-fileEvents
			if e.EffectType != watcher.EffectTypeModify {
				t.Errorf("expected modify event, got %v", e.EffectType)
			}
		}()

		w := watcher.NewWatcher(dir, func(e *watcher.Event) {
			if strings.HasPrefix(e.PathName, "s/self/live@") {
				ready <- struct{}{}

				return
			}

			if strings.HasSuffix(e.PathName, "test.txt") {
				fileEvents <- e
			}
		})
		t.Cleanup(w.Close)

		// Wait for the watcher to be fully started
		<-ready

		if err := os.WriteFile(filepath.Join(dir, "test.txt"), []byte("test"), 0644); err != nil {
			t.Fatalf("failed to write file: %v", err)
		}

		synctest.Wait()
	})
}
