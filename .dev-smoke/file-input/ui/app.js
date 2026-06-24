import { h, render } from 'preact';
import { useMemo, useState } from 'preact/hooks';

const styles = {
  page: {
    width: '100%',
    height: '100%',
    minHeight: 0,
    boxSizing: 'border-box',
    overflow: 'hidden',
    padding: 18,
    fontFamily: 'Segoe UI, Arial, sans-serif',
    color: '#172033',
    background: '#f5f7fb'
  },
  shell: {
    height: '100%',
    display: 'grid',
    gridTemplateRows: 'auto 1fr',
    gap: 14,
    overflow: 'hidden'
  },
  header: {
    display: 'grid',
    gridTemplateColumns: '1fr auto',
    gap: 16,
    alignItems: 'end',
    paddingBottom: 12,
    borderBottom: '1px solid #d7deea'
  },
  title: {
    margin: 0,
    fontSize: 24,
    lineHeight: 1.18,
    letterSpacing: 0
  },
  subtitle: {
    margin: '6px 0 0',
    color: '#526172',
    fontSize: 13,
    lineHeight: 1.45
  },
  badge: {
    display: 'inline-flex',
    alignItems: 'center',
    minHeight: 28,
    padding: '0 10px',
    borderRadius: 6,
    border: '1px solid #c5d1e2',
    color: '#234064',
    background: '#eef4fb',
    fontSize: 12,
    whiteSpace: 'nowrap'
  },
  grid: {
    minHeight: 0,
    display: 'grid',
    gridTemplateColumns: 'minmax(300px, 420px) minmax(0, 1fr)',
    gap: 14,
    overflow: 'hidden'
  },
  panel: {
    minHeight: 0,
    border: '1px solid #d7deea',
    borderRadius: 8,
    background: '#ffffff',
    overflow: 'hidden',
    display: 'grid',
    gridTemplateRows: 'auto 1fr'
  },
  panelHeader: {
    padding: '12px 14px',
    borderBottom: '1px solid #e4e9f1',
    display: 'grid',
    gap: 4
  },
  panelTitle: {
    margin: 0,
    fontSize: 15,
    lineHeight: 1.2,
    color: '#1d2a3d'
  },
  panelNote: {
    margin: 0,
    color: '#667586',
    fontSize: 12,
    lineHeight: 1.4
  },
  hint: {
    border: '1px solid #d7deea',
    borderRadius: 6,
    background: '#f8fbff',
    padding: 10,
    color: '#344861',
    fontSize: 12,
    lineHeight: 1.45
  },
  body: {
    minHeight: 0,
    overflow: 'auto',
    padding: 14,
    display: 'grid',
    alignContent: 'start',
    gap: 12
  },
  field: {
    display: 'grid',
    gap: 7
  },
  label: {
    color: '#26374e',
    fontWeight: 650,
    fontSize: 13
  },
  input: {
    boxSizing: 'border-box',
    width: '100%',
    minHeight: 36,
    padding: '8px 10px',
    border: '1px solid #b9c7d9',
    borderRadius: 6,
    background: '#ffffff',
    color: '#172033'
  },
  dropZone: {
    minHeight: 96,
    border: '1px dashed #7b8faa',
    borderRadius: 8,
    background: '#f8fbff',
    color: '#394b63',
    display: 'grid',
    alignContent: 'center',
    justifyItems: 'center',
    gap: 6,
    padding: 14,
    textAlign: 'center'
  },
  buttonRow: {
    display: 'flex',
    gap: 8,
    flexWrap: 'wrap'
  },
  button: {
    minHeight: 34,
    border: '1px solid #9eb1c8',
    borderRadius: 6,
    padding: '7px 11px',
    color: '#17324e',
    background: '#edf4fc',
    fontWeight: 650,
    cursor: 'pointer'
  },
  dangerButton: {
    minHeight: 34,
    border: '1px solid #e0aaa4',
    borderRadius: 6,
    padding: '7px 11px',
    color: '#6b2b24',
    background: '#fff1ef',
    fontWeight: 650,
    cursor: 'pointer'
  },
  report: {
    display: 'grid',
    gap: 10
  },
  row: {
    border: '1px solid #e1e7ef',
    borderRadius: 6,
    padding: 10,
    display: 'grid',
    gap: 5,
    background: '#fbfcfe'
  },
  rowTitle: {
    fontWeight: 700,
    color: '#26374e',
    fontSize: 13
  },
  code: {
    fontFamily: 'Consolas, monospace',
    fontSize: 12,
    color: '#253247',
    whiteSpace: 'pre-wrap',
    overflowWrap: 'anywhere',
    lineHeight: 1.42
  },
  ok: {
    color: '#13795b',
    fontWeight: 700
  },
  fail: {
    color: '#b42318',
    fontWeight: 700
  }
};

let lastDropFileList = null;

function installDocumentShell() {
  Object.assign(document.documentElement.style, {
    width: '100%',
    height: '100%',
    margin: '0',
    overflow: 'hidden'
  });
  Object.assign(document.body.style, {
    width: '100%',
    height: '100%',
    margin: '0',
    overflow: 'hidden'
  });
}

function mountRoot() {
  let root = document.getElementById('root');
  if (!root) {
    root = document.createElement('div');
    root.id = 'root';
    document.body.appendChild(root);
  }
  Object.assign(root.style, {
    width: '100%',
    height: '100%',
    overflow: 'hidden'
  });
  return root;
}

function stringify(value) {
  if (value === null) return 'null';
  if (typeof value === 'undefined') return 'undefined';
  if (typeof value === 'string') return value;
  try {
    return JSON.stringify(value, null, 2);
  } catch (error) {
    return String(value);
  }
}

function listToRows(files) {
  const count = files ? Number(files.length || 0) : 0;
  const rows = [];
  for (let index = 0; index < count; index += 1) {
    const file = files[index];
    rows.push({
      index,
      name: file && file.name,
      type: file && file.type,
      size: file && file.size,
      webkitRelativePath: file && file.webkitRelativePath,
      normalizedPath: file && file.normalizedPath,
      pathType: typeof (file && file.path),
      isDirectoryType: typeof (file && file.isDirectory),
      tag: Object.prototype.toString.call(file)
    });
  }
  return rows;
}

function summarizeInput(input) {
  const files = input.files;
  const descriptor = files ? Object.getOwnPropertyDescriptor(files, 'length') : null;
  return {
    value: input.value,
    length: files ? files.length : null,
    listTag: Object.prototype.toString.call(files),
    lengthWritable: descriptor ? descriptor.writable === true : null,
    rows: listToRows(files)
  };
}

function createEmptyInputProbe() {
  const input = document.createElement('input');
  input.setAttribute('type', 'file');
  const files = input.files;
  const beforeTag = Object.prototype.toString.call(files);
  const descriptor = Object.getOwnPropertyDescriptor(files, 'length');
  let lengthWriteRejected = false;
  let indexWriteRejected = false;
  try {
    files.length = 42;
  } catch (error) {
    lengthWriteRejected = true;
  }
  try {
    files[0] = { name: 'mutated.txt' };
  } catch (error) {
    indexWriteRejected = true;
  }

  let badSetterRejected = false;
  try {
    input.files = { length: 1 };
  } catch (error) {
    badSetterRejected = String(error && error.message).indexOf('FileList') >= 0;
  }

  return {
    ok: Boolean(files) &&
      beforeTag === '[object FileList]' &&
      files.length === 0 &&
      files.item(0) === null &&
      descriptor &&
      descriptor.writable === false &&
      badSetterRejected,
    detail: {
      listTag: beforeTag,
      emptyLength: files.length,
      item0: files.item(0),
      lengthWritable: descriptor && descriptor.writable,
      lengthConfigurable: descriptor && descriptor.configurable,
      lengthWriteRejected,
      indexWriteRejected,
      nonFileElementFiles: document.createElement('div').files,
      badSetterRejected,
      dataTransferConstructor: typeof DataTransfer
    }
  };
}

function ReportRow({ title, data, good }) {
  return h('article', { style: styles.row },
    h('div', { style: styles.rowTitle },
      title,
      typeof good === 'boolean'
        ? h('span', { style: good ? styles.ok : styles.fail }, good ? '  PASS' : '  FAIL')
        : null
    ),
    h('pre', { style: styles.code }, stringify(data))
  );
}

function FilePicker({ id, reportKey, label, attrs, onFiles }) {
  return h('label', { style: styles.field },
    h('span', { style: styles.label }, label),
    h('input', {
      id,
      type: 'file',
      style: styles.input,
      onInput: (event) => onFiles(reportKey, event.currentTarget),
      onChange: (event) => onFiles(reportKey, event.currentTarget),
      ...attrs
    })
  );
}

function App() {
  const emptyProbe = useMemo(createEmptyInputProbe, []);
  const [reports, setReports] = useState({
    single: { status: 'not selected' },
    multi: { status: 'not selected' },
    folder: { status: 'not selected' },
    drop: { status: 'drop files from Explorer here' },
    assign: { status: 'waiting for dropped FileList' }
  });
  const [dropActive, setDropActive] = useState(false);

  function updateReport(name, value) {
    setReports((current) => ({ ...current, [name]: value }));
  }

  function handleFiles(name, input) {
    updateReport(name, summarizeInput(input));
  }

  function handleDrop(event) {
    event.preventDefault();
    setDropActive(false);
    const transfer = event.dataTransfer;
    const files = transfer && transfer.files;
    lastDropFileList = files || null;
    updateReport('drop', {
      transferTag: Object.prototype.toString.call(transfer),
      types: transfer && transfer.types,
      filesTag: Object.prototype.toString.call(files),
      length: files ? files.length : null,
      rows: listToRows(files)
    });
  }

  function assignDroppedFiles() {
    const input = document.getElementById('single-input');
    if (!input || !lastDropFileList) {
      updateReport('assign', { ok: false, reason: 'No dropped FileList is available yet.' });
      return;
    }
    try {
      input.files = lastDropFileList;
      const summary = summarizeInput(input);
      updateReport('assign', { ok: true, assignedTo: '#single-input', summary });
      updateReport('single', summary);
    } catch (error) {
      updateReport('assign', { ok: false, error: String(error && error.message ? error.message : error) });
    }
  }

  function clearSingleFiles() {
    const input = document.getElementById('single-input');
    if (!input) return;
    input.files = null;
    const summary = summarizeInput(input);
    updateReport('single', summary);
    updateReport('assign', { ok: true, action: 'single input cleared', summary });
  }

  return h('main', { id: 'file-input-smoke-root', style: styles.page },
    h('div', { style: styles.shell },
      h('header', { style: styles.header },
        h('div', null,
          h('h1', { style: styles.title }, 'File input / FileList dev smoke'),
          h('p', { style: styles.subtitle }, 'Tests real MBink UI runtime behavior for input.files, file pickers, folder selection, drag/drop FileList, and privacy-shaped File objects.')
        ),
        h('span', { id: 'runtime-badge', style: styles.badge }, 'mbink-ui-dev runtime')
      ),
      h('section', { style: styles.grid },
        h('section', { style: styles.panel },
          h('div', { style: styles.panelHeader },
            h('h2', { style: styles.panelTitle }, 'Controls'),
            h('p', { style: styles.panelNote }, 'Use native picker dialogs or drag files from Explorer into the drop area.')
          ),
          h('div', { style: styles.body },
            h('div', { id: 'normalized-path-hint', style: styles.hint },
              'After selecting a file, read the real path with input.files[0].normalizedPath.'
            ),
            h(FilePicker, {
              id: 'single-input',
              reportKey: 'single',
              label: 'Single file with accept=.txt,image/*',
              attrs: { accept: '.txt,image/*' },
              onFiles: handleFiles
            }),
            h(FilePicker, {
              id: 'multi-input',
              reportKey: 'multi',
              label: 'Multiple files',
              attrs: { multiple: true },
              onFiles: handleFiles
            }),
            h(FilePicker, {
              id: 'folder-input',
              reportKey: 'folder',
              label: 'Folder selection with webkitdirectory',
              attrs: { webkitdirectory: '', directory: '' },
              onFiles: handleFiles
            }),
            h('div', {
              id: 'drop-zone',
              style: {
                ...styles.dropZone,
                borderColor: dropActive ? '#2c7be5' : styles.dropZone.borderColor,
                background: dropActive ? '#eaf3ff' : styles.dropZone.background
              },
              onDragEnter: (event) => {
                event.preventDefault();
                setDropActive(true);
              },
              onDragOver: (event) => {
                event.preventDefault();
                setDropActive(true);
              },
              onDragLeave: (event) => {
                event.preventDefault();
                setDropActive(false);
              },
              onDrop: handleDrop
            },
              h('strong', null, 'Drop files or a folder here'),
              h('span', null, 'Then press Assign drop FileList to single input.')
            ),
            h('div', { style: styles.buttonRow },
              h('button', { id: 'assign-drop-button', style: styles.button, onClick: assignDroppedFiles }, 'Assign drop FileList'),
              h('button', { id: 'clear-single-button', style: styles.dangerButton, onClick: clearSingleFiles }, 'Clear single input')
            )
          )
        ),
        h('section', { style: styles.panel },
          h('div', { style: styles.panelHeader },
            h('h2', { style: styles.panelTitle }, 'Live reports'),
            h('p', { style: styles.panelNote }, 'Selected files expose File.normalizedPath for MBink real paths; File.path / File.isDirectory stay undefined.')
          ),
          h('div', { id: 'report-panel', style: { ...styles.body, ...styles.report } },
            h(ReportRow, { title: 'Automatic empty FileList probe', data: emptyProbe.detail, good: emptyProbe.ok }),
            h(ReportRow, { title: 'Single input', data: reports.single }),
            h(ReportRow, { title: 'Multiple input', data: reports.multi }),
            h(ReportRow, { title: 'Folder input', data: reports.folder }),
            h(ReportRow, { title: 'Drop dataTransfer.files', data: reports.drop }),
            h(ReportRow, { title: 'input.files = dropped FileList', data: reports.assign })
          )
        )
      )
    )
  );
}

installDocumentShell();
render(h(App), mountRoot());
