import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'bindings', 'python'))

from mbink import App
from bindings.python.examples.select_dropdown_verify import HTML


def main():
    app = App('MBink Select Exit Probe', 980, 820, gpu=False)
    app.load_html(HTML)

    fired = {'done': False}

    @app.on_update
    def _update(_dt):
        if not fired['done']:
            fired['done'] = True
            app.stop()

    app.run()
    print('select probe done', flush=True)


if __name__ == '__main__':
    main()
