import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'bindings', 'python'))

from mbink import App


def main():
    app = App('MBink Smoke Exit', 320, 240, gpu=False)
    app.load_html('<!DOCTYPE html><html><body>ok</body></html>')

    fired = {'done': False}

    @app.on_update
    def _update(_dt):
        if not fired['done']:
            fired['done'] = True
            app.stop()

    app.run()
    print('done', flush=True)


if __name__ == '__main__':
    main()
