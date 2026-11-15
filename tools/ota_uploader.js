(() => {
    function addFirmwareUploader() {
        const heading = Array.from(document.querySelectorAll('.card h5')).find(element =>
            ['Firmware Update', 'Firmware-Update'].includes(element.textContent.trim())
        );
        const card = heading && heading.closest('.card');

        if (!card || document.getElementById('wordclock-ota-uploader')) return;

        const panel = document.createElement('div');
        panel.id = 'wordclock-ota-uploader';
        panel.style.cssText = 'background-color:unset;width:100%;text-align:left;padding:8px 3px;';

        const picker = document.createElement('input');
        picker.type = 'file';
        picker.accept = '.bin,application/octet-stream';
        picker.hidden = true;

        const fileField = document.createElement('div');
        fileField.style.cssText = 'display:flex;flex-direction:row;align-items:center;justify-content:flex-start;text-align:left;color:black;background: rgba(255, 255, 255, 0.8);box-shadow: inset 0 0 6px rgba(0, 0, 0, 0.3);width:calc(100% - 4px);height:36px;margin:0 0 8px 2px;padding:6px;gap:8px;box-sizing:border-box;border:1px solid #808080;border-radius:4px';
        const chooseFileButton = document.createElement('button');
        chooseFileButton.type = 'button';
        chooseFileButton.textContent = 'Choose firmware file';
        chooseFileButton.style.cssText = 'flex:0 0 auto;width:auto;height:28px;min-height:0;margin: 3px 3px;padding:2px 8px;font-size:.85em;line-height:1.2;box-sizing:border-box;cursor:pointer';
        chooseFileButton.addEventListener('click', () => picker.click());

        const filename = document.createElement('span');
        filename.textContent = 'No file selected';
        filename.style.cssText = 'flex:0 1 auto;min-width:0;text-align:center;overflow-wrap:anywhere;font-weight:300;font-size:1em';
        picker.addEventListener('change', () => {
            filename.textContent = picker.files && picker.files[0]
                ? picker.files[0].name
                : 'No file selected';
        });
        fileField.append(chooseFileButton, filename);

        const button = document.createElement('button');
        button.type = 'button';
        button.textContent = 'Upload and install';
        button.style.cssText = 'width:100%;margin:3px 0;box-sizing:border-box;cursor:pointer';

        const progress = document.createElement('progress');
        progress.max = 100;
        progress.value = 0;
        progress.hidden = true;
        progress.style.cssText = 'display:block;width:calc(100% - 2px);height:18px;margin:8px 0 8px;box-sizing:border-box';

        const status = document.createElement('p');
        status.setAttribute('role', 'status');
        status.textContent = 'Choose a .bin image to begin.';
        status.style.cssText = 'background-color:unset;text-align:left;padding:0;margin:0 0 8px 0px;top:0;font-weight:300;font-size:.9em;width:calc(100% - 4px);box-sizing:border-box';

        button.addEventListener('click', () => {
            const file = picker.files && picker.files[0];
            if (!file || !file.name.toLowerCase().endsWith('.bin')) {
                status.textContent = 'Select a .bin application image.';
                return;
            }

            button.disabled = true;
            picker.disabled = true;
            chooseFileButton.disabled = true;
            status.textContent = 'Starting firmware upload…';
            progress.hidden = false;
            progress.value = 0;

            const form = new FormData();
            form.append('firmware', file, file.name);
            const request = new XMLHttpRequest();
            request.open('POST', '/update');
            request.upload.onprogress = event => {
                if (event.lengthComputable) {
                    progress.value = Math.round(event.loaded * 100 / event.total);
                    status.textContent = 'Uploading ' + progress.value + '%';
                }
            };
            request.upload.onload = () => {
                progress.value = 100;
                status.textContent = 'Upload complete. Validating firmware…';
            };
            request.onload = () => {
                status.textContent = request.responseText || ('Update failed (' + request.status + ').');
                if (request.status !== 200) {
                    button.disabled = false;
                    picker.disabled = false;
                    chooseFileButton.disabled = false;
                }
            };
            request.onerror = () => {
                status.textContent = 'Connection lost. The device may be restarting; wait and reconnect.';
                button.disabled = false;
                picker.disabled = false;
                chooseFileButton.disabled = false;
            };
            request.send(form);
        });

        panel.append(fileField, picker, button, progress, status);
        card.appendChild(panel);
    }

    const observer = new MutationObserver(addFirmwareUploader);
    observer.observe(document.documentElement, { childList: true, subtree: true });
    addFirmwareUploader();
})();

/* Scheduler controls are raw label HTML, so their transport is independent of ESPUI callbacks. */
(() => {
    const slotCount = 8;
    const fields = ['active', 'on', 'from', 'to', 'time', 'color', 'message', 'range', 'value'];
    const presets = [[0, 100], [20, 50]];
    const swatches = ['#FFFFFF', '#C0C0C0', '#808080', '#000000', '#FF0000', '#800000',
        '#FFFF00', '#808000', '#00FF00', '#00FFFF', '#0000FF', '#FF00FF'];
    const pickers = new Map();
    const encoder = new TextEncoder();
    let pickerRetry;
    // Keep an owned baseline, not live nodes or the most recent Save/setRows values.
    // Default DOM values also protect against edits made before the observer first runs.
    let initialRows;
    let rowTemplate;
    const initialized = new WeakSet();

    // Override ESPUI's full-width/block inputs only inside scheduler cells. Flex
    // groups center checkbox/label and slider/output pairs as a single unit.
    const style = document.createElement('style');
    style.textContent = `
        [data-scheduler] table { border-collapse:collapse; }
        [data-scheduler] table th, [data-scheduler] table td {
            text-align:center; vertical-align:middle; padding:3px 4px;
        }
        [data-scheduler] tbody tr[data-slot]:not(:last-of-type) > td {
            border-bottom:1px solid #808080;
        }
        [data-scheduler] [data-scheduler-cell] {
            display:flex; align-items:center; justify-content:center; gap:6px; min-height:26px;
        }
        [data-scheduler] [data-scheduler-cell] input,
        [data-scheduler] [data-scheduler-cell] select {
            display:inline-block; flex:0 0 auto; height:26px; margin:0; padding:2px;
            box-sizing:border-box; text-align:center; vertical-align:middle;
        }
        [data-scheduler] [data-color-anchor] {
            width:100px; font-family:monospace; cursor:pointer;
        }
        [data-scheduler] [data-color-status]:empty { display:none; }
        /* Firmware owns the icons; these scoped rules override ESPUI's large
           button defaults without affecting other cards or Pickr actions. */
        [data-scheduler] [data-scheduler-toolbar] {
            display:flex; justify-content:flex-end; align-items:center; flex-wrap:wrap;
            gap:6px; width:100%; margin:8px 0; box-sizing:border-box; white-space:normal;
        }
        [data-scheduler] [data-scheduler-toolbar] > button {
            display:inline-flex; align-items:center; justify-content:center; flex:0 0 32px;
            width:32px; height:32px; min-width:32px; min-height:32px; max-width:32px; max-height:32px;
            margin:0; padding:0; box-sizing:border-box; line-height:1;
            color:#fff; background:#444857; border:1px solid #808080; border-radius:4px;
            box-shadow:none; cursor:pointer;
        }
        [data-scheduler] [data-scheduler-toolbar] > button:hover:not(:disabled) { background:#565c6e; }
        [data-scheduler] [data-scheduler-toolbar] > button:focus-visible { outline:2px solid #fff; outline-offset:2px; }
        [data-scheduler] [data-scheduler-toolbar] > button:disabled { opacity:.45; cursor:not-allowed; }
        [data-scheduler] [data-scheduler-toolbar] svg {
            display:block; width:18px; height:18px; fill:none; stroke:currentColor;
            stroke-width:2; stroke-linecap:round; stroke-linejoin:round;
        }
        [data-scheduler] [data-scheduler-status] { display:block; white-space:normal; }
        @media (max-width:600px) {
            [data-scheduler] [data-scheduler-toolbar] > button {
                flex-basis:36px; width:36px; height:36px;
                min-width:36px; min-height:36px; max-width:36px; max-height:36px;
            }
        }
        /* Pickr emits hidden format buttons; host ESPUI input rules can unhide
           them. Keep this override exclusive to scheduler popups. */
        .wordclock-scheduler-picker .pcr-interaction .pcr-type { display:none !important; }
    `;
    document.head.append(style);

    function capacity(root) {
        const count = Number(root.dataset.slotCount || slotCount);
        return Number.isInteger(count) && count >= 0 && count <= slotCount ? count : slotCount;
    }

    function prepare(root) {
        if (!root) throw new Error('Scheduler table not available.');
        if (initialized.has(root)) return;
        const template = root.querySelector('template[data-scheduler-row-template]');
        if (!template || !template.content.firstElementChild) throw new Error('Scheduler template not available.');
        if (!initialRows) {
            rowTemplate = template.content.firstElementChild.cloneNode(true);
            initialRows = Array.from(root.querySelectorAll('tbody tr[data-slot]'), element => {
                const original = element.cloneNode(true);
                original.querySelectorAll('input').forEach(input => {
                    input.value = input.defaultValue;
                    input.checked = input.defaultChecked;
                });
                original.querySelectorAll('select').forEach(select => {
                    const selected = Array.from(select.options).find(option => option.defaultSelected);
                    select.value = selected ? selected.value : select.options[0].value;
                });
                return Object.freeze(getRow(original));
            });
            Object.freeze(initialRows);
        }
        root.querySelectorAll('tbody [data-row-selected]').forEach(input => { input.checked = false; });
        initialized.add(root);
        updateActions(root);
    }

    // Selection is browser-only state, independent of Active and the scheduler schema.
    // Keep structural actions unavailable during Save, then restore selection/count limits.
    function updateActions(root) {
        const busy = root.querySelector('[data-action="save-scheduler"]').disabled;
        const count = root.querySelectorAll('tbody tr[data-slot]').length;
        const selected = root.querySelector('tbody tr[data-slot] [data-row-selected]:checked');
        for (const button of root.querySelectorAll('[data-action]')) {
            switch (button.dataset.action) {
                case 'add-scheduler-row': button.disabled = busy || count >= capacity(root); break;
                case 'remove-selected-scheduler-rows': button.disabled = busy || !selected; break;
                case 'reload-scheduler': button.disabled = busy; break;
            }
        }
    }

    function disposePicker(element) {
        const anchor = element.matches('[data-color-anchor]') ? element : element.querySelector('[data-color-anchor]');
        const state = pickers.get(anchor);
        if (!state) return;
        // Invalidate callbacks before hide/destroy (including a pending deferred init).
        pickers.delete(anchor);
        state.ready = false;
        state.open = false;
        state.picker.hide();
        state.picker.destroyAndRemove();
    }

    function validMessage(message) {
        if (typeof message !== 'string' || /[\u0000-\u001f\u007f-\u009f]/.test(message)) return false;
        // TextEncoder replaces lone surrogates, whereas the device rejects malformed UTF-8.
        for (const character of message) {
            const code = character.codePointAt(0);
            if (code >= 0xD800 && code <= 0xDFFF) return false;
        }
        return encoder.encode(message).length <= 64;
    }

    function updateValue(element, clamp = false) {
        const range = element.querySelector('[data-field="range"]');
        const slider = element.querySelector('[data-field="value"]');
        const output = element.querySelector('[data-value-output]');
        const bounds = presets[Number(range.value)];
        if (!bounds || range.value === '') return;
        const value = Number(slider.value);
        slider.min = bounds[0];
        slider.max = bounds[1];
        if (clamp) slider.value = Math.max(bounds[0], Math.min(bounds[1], value));
        if (output && output.textContent !== slider.value) output.textContent = slider.value;
    }

    // Silent updates plus a guard prevent Pickr events recursively committing draft colors.
    function syncPicker(element) {
        const anchor = element.querySelector('[data-color-anchor]');
        const color = element.querySelector('[data-field="color"]').value;
        if (!anchor || !/^#[0-9a-f]{6}$/i.test(color)) return;
        anchor.value = '0x' + color.slice(1).toUpperCase();
        anchor.style.backgroundColor = color;
        // WCAG relative luminance: choose the larger black/white contrast ratio,
        // not the RGB integer's magnitude (which misclassifies green and blue).
        const channels = color.slice(1).match(/../g).map(hex => {
            const value = parseInt(hex, 16) / 255;
            return value <= 0.04045 ? value / 12.92 : ((value + 0.055) / 1.055) ** 2.4;
        });
        const luminance = channels[0] * 0.2126 + channels[1] * 0.7152 + channels[2] * 0.0722;
        anchor.style.color = luminance > 0.179 ? '#000000' : '#FFFFFF';
        const state = pickers.get(anchor);
        if (!state || !state.ready) return;
        state.syncing = true;
        try {
            state.picker.setColor(color, true);
            state.picker.applyColor(true);
        } finally {
            state.syncing = false;
        }
    }

    // Use the bundled Pickr API (https://github.com/simonwep/pickr); keep anchors stable.
    function refreshPickers() {
        for (const [anchor, state] of pickers) {
            if (!anchor.isConnected) {
                disposePicker(anchor);
            }
        }
        document.querySelectorAll('[data-scheduler]').forEach(root => {
            try { prepare(root); } catch { /* Wait for the complete firmware label/template. */ }
        });
        clearTimeout(pickerRetry);
        pickerRetry = undefined;
        const available = window.Pickr && typeof window.Pickr.create === 'function';
        let waitingForLibrary = false;
        document.querySelectorAll('[data-scheduler] [data-color-anchor]').forEach(anchor => {
            if (pickers.has(anchor)) return;
            const element = anchor.closest('tr[data-slot]');
            const input = element && element.querySelector('[data-field="color"]');
            if (!input) return;
            // Never expose a native color input; ESPUI's input display rules override [hidden].
            input.type = 'text';
            input.hidden = true;
            input.style.setProperty('display', 'none', 'important');
            syncPicker(element);
            anchor.hidden = false;
            anchor.style.display = 'inline-block';
            anchor.disabled = true;
            const feedback = element.querySelector('[data-color-status]');
            const report = message => {
                if (feedback && feedback.textContent !== message) feedback.textContent = message;
            };
            if (!available) {
                report('Color picker unavailable (ESPUI Pickr not loaded).');
                waitingForLibrary = true;
                return;
            }
            const state = { picker: null, syncing: false, ready: false, open: false };
            try {
                state.picker = window.Pickr.create({
                    el: anchor, useAsButton: true, theme: 'monolith', default: input.value,
                    appClass: 'wordclock-scheduler-picker',
                    i18n: { 'btn:toggle': 'Choose color' },
                    lockOpacity: true, defaultRepresentation: 'HEX', comparison: true,
                    swatches, components: { preview: true, opacity: false, hue: true,
                        interaction: { hex: false, rgba: false, hsla: false, hsva: false,
                            cmyk: false, input: true, save: true, cancel: true } }
                });
            } catch (error) {
                report('Color picker unavailable (initialization failed).');
                return;
            }
            pickers.set(anchor, state);
            state.picker.on('init', () => {
                if (pickers.get(anchor) !== state) return;
                state.ready = true;
                syncPicker(element);
                anchor.disabled = false;
                report('');
            }).on('show', () => {
                if (!state.ready) { state.picker.hide(); return; }
                for (const other of pickers.values()) {
                    if (other !== state && other.open) other.picker.hide();
                }
                state.open = true;
                syncPicker(element);
            }).on('save', color => {
                // Pickr initialization and setColor() can emit save without a user commit.
                if (!state.ready || !state.open || state.syncing || !color) return;
                input.value = color.toHEXA().toString().slice(0, 7);
                syncPicker(element);
                state.picker.hide();
            }).on('cancel', () => state.picker.hide()).on('hide', () => {
                state.open = false;
                syncPicker(element);
            });
        });
        // A deferred loader can publish Pickr after its script's load event, with no DOM changes.
        // Retry only while unenhanced scheduler rows exist; stop once all have an instance.
        if (waitingForLibrary) pickerRetry = setTimeout(refreshPickers, 250);
    }

    // Reject incomplete rows before updating controls or submitting anything to the device.
    function validateRow(row) {
        const limits = { slot: slotCount - 1, from: 6, to: 6, hour: 23, minute: 59 };
        if (!row || typeof row.active !== 'boolean' || typeof row.on !== 'boolean') {
            throw new Error('Active and On must be boolean values.');
        }
        for (const [field, max] of Object.entries(limits)) {
            if (!Number.isInteger(row[field]) || row[field] < 0 || row[field] > max) {
                throw new Error('Invalid ' + field + ' for slot ' + row.slot + '.');
            }
        }
        if (typeof row.color !== 'string' || !/^#[0-9a-f]{6}$/i.test(row.color)) {
            throw new Error('Color must be #RRGGBB.');
        }
        if (!validMessage(row.message)) throw new Error('Message must be at most 64 UTF-8 bytes without control characters.');
        if (!Number.isInteger(row.range) || !presets[row.range]) throw new Error('Unknown range preset.');
        const [min, max] = presets[row.range];
        if (!Number.isInteger(row.value) || row.value < min || row.value > max) throw new Error('Value outside selected range.');
        return row;
    }

    function getRow(element) {
        const field = name => element.querySelector('[data-field="' + name + '"]');
        const number = name => {
            const input = field(name);
            if (!input || input.value.trim() === '') throw new Error('Missing ' + name + '.');
            return Number(input.value);
        };
        // Time inputs expose HH:MM regardless of the browser's localized display format.
        const time = field('time');
        if (!time || !/^([01]\d|2[0-3]):[0-5]\d$/.test(time.value)) {
            throw new Error('Enter a valid time for slot ' + element.dataset.slot + '.');
        }
        const [hour, minute] = time.value.split(':').map(Number);
        return validateRow({
            slot: Number(element.dataset.slot),
            active: field('active').checked,
            on: field('on').checked,
            from: number('from'),
            to: number('to'),
            hour,
            minute,
            color: field('color').value.toUpperCase(),
            message: field('message').value,
            range: number('range'),
            value: number('value')
        });
    }

    // Setting a row changes browser values only; it neither saves nor triggers control events.
    function setRow(element, row) {
        validateRow(row);
        if (Number(element.dataset.slot) !== row.slot) throw new Error('Slot mismatch.');
        const inputs = fields.map(name => element.querySelector('[data-field="' + name + '"]'));
        if (inputs.some(input => !input)) throw new Error('Incomplete scheduler row.');
        fields.forEach((name, index) => {
            if (name === 'active' || name === 'on') inputs[index].checked = row[name];
            else if (name === 'time') inputs[index].value = String(row.hour).padStart(2, '0') + ':' + String(row.minute).padStart(2, '0');
            else if (name !== 'value') inputs[index].value = String(row[name]);
        });
        // Repair bounds before assigning the value, even if current controls are invalid.
        updateValue(element);
        element.querySelector('[data-field="value"]').value = row.value;
        updateValue(element);
        syncPicker(element);
    }

    function getElements(root) {
        prepare(root);
        const elements = Array.from(root.querySelectorAll('tbody tr[data-slot]'));
        const slots = elements.map(element => Number(element.dataset.slot));
        if (elements.length > capacity(root) || new Set(slots).size !== slots.length ||
            slots.some(slot => !Number.isInteger(slot) || slot < 0 || slot >= capacity(root))) {
            throw new Error('Expected zero to ' + capacity(root) + ' unique scheduler slots.');
        }
        return elements;
    }

    function getRows(root = document.querySelector('[data-scheduler]')) {
        return getElements(root).map(getRow);
    }

    // Slot numbers describe current positions, not row identity. Keep the nodes
    // (and anchor-keyed Pickr state) intact when only their position changes.
    function setSlot(element, slot) {
        element.dataset.slot = slot;
        const label = element.querySelector('[data-slot-label]');
        if (label.textContent !== String(slot)) label.textContent = slot;
        element.querySelector('[data-row-selected]').setAttribute('aria-label', 'Select slot ' + slot);
    }

    // Validate the complete batch before applying any row, avoiding partial updates on bad data.
    function setRows(rows, root = document.querySelector('[data-scheduler]'), recreate = false) {
        if (!root) throw new Error('Scheduler table not available.');
        if (!Array.isArray(rows) || rows.length > capacity(root)) throw new Error('Too many scheduler rows.');
        const slots = new Set();
        for (const row of rows) {
            validateRow(row);
            if (row.slot >= capacity(root) || slots.has(row.slot)) throw new Error('Duplicate or invalid scheduler slot.');
            slots.add(row.slot);
        }
        const elements = getElements(root);
        const targets = rows.map(row => {
            const element = (!recreate && elements.find(element => Number(element.dataset.slot) === row.slot)) || rowTemplate.cloneNode(true);
            if (!element.querySelector('[data-slot-label]')) throw new Error('Missing scheduler slot label.');
            if (!element.querySelector('[data-row-selected]')) throw new Error('Missing scheduler row selection.');
            for (const name of fields) {
                if (!element.querySelector('[data-field="' + name + '"]')) throw new Error('Incomplete scheduler row.');
            }
            return element;
        });
        // All data and target shapes have been checked before any live DOM mutation.
        elements.filter(element => !targets.includes(element)).forEach(element => {
            disposePicker(element);
            element.remove();
        });
        targets.forEach((element, index) => {
            setSlot(element, rows[index].slot);
            const selection = element.querySelector('[data-row-selected]');
            selection.checked = false;
            setRow(element, rows[index]);
            root.querySelector('tbody').append(element);
        });
        updateActions(root);
        refreshPickers();
    }

    function reload(root = document.querySelector('[data-scheduler]')) {
        prepare(root);
        setRows(initialRows, root, true);
        root.querySelector('[data-scheduler-status]').textContent = 'Initial rows restored; browser edits discarded. Nothing saved.';
    }

    function addRow(root = document.querySelector('[data-scheduler]')) {
        const rows = getRows(root);
        if (rows.length >= capacity(root)) throw new Error('Maximum ' + capacity(root) + ' rows reached.');
        const used = new Set(rows.map(row => row.slot));
        let slot = 0;
        while (used.has(slot)) slot++;
        setRows([...rows, { slot, active: false, on: false, from: 0, to: 0, hour: 0, minute: 0,
            color: '#FFFFFF', message: '', range: 0, value: 0 }], root);
    }

    function removeRow(element) {
        const root = element.closest('[data-scheduler]');
        prepare(root);
        disposePicker(element);
        element.remove();
        // Do not round-trip values through setRows: incomplete browser edits and
        // open/draft Pickr state on surviving rows must remain untouched.
        root.querySelectorAll('tbody tr[data-slot]').forEach(setSlot);
        updateActions(root);
        if (!root.querySelector('tbody tr[data-slot]')) {
            clearTimeout(pickerRetry);
            pickerRetry = undefined;
        }
    }

    // Snapshot the targets before disposing Pickrs/removing nodes so adjacent rows cannot be skipped.
    function removeSelectedRows(root = document.querySelector('[data-scheduler]')) {
        prepare(root);
        if (root.querySelector('[data-action="save-scheduler"]').disabled) return;
        const selected = Array.from(root.querySelectorAll('tbody tr[data-slot]'))
            .filter(element => element.querySelector('[data-row-selected]').checked);
        selected.forEach(removeRow);
        updateActions(root);
    }

    async function save(root = document.querySelector('[data-scheduler]')) {
        if (!root) throw new Error('Scheduler table not available.');
        const button = root.querySelector('[data-action="save-scheduler"]');
        const status = root.querySelector('[data-scheduler-status]');
        if (button.disabled) return;
        const abort = new AbortController();
        let timeout;
        try {
            const rows = getRows(root);
            button.disabled = true;
            updateActions(root);
            status.textContent = 'Submitting displayed rows for logging only…';
            timeout = setTimeout(() => abort.abort(), 10000);
            const response = await fetch('/scheduler/save', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ rows }),
                signal: abort.signal
            });
            const message = await response.text();
            if (!response.ok) throw new Error(message || ('Submission failed (' + response.status + ').'));
            status.textContent = message;
        } catch (error) {
            status.textContent = error.name === 'AbortError'
                ? 'Request timed out; logging may already have occurred.' : error.message;
        } finally {
            clearTimeout(timeout);
            button.disabled = false;
            updateActions(root);
        }
    }

    window.WordClockScheduler = { getRow, setRow, getRows, setRows, save, reload, addRow, removeRow, removeSelectedRows };

    // Delegation survives ESPUI replacing the label or loading it after this script runs.
    document.addEventListener('keydown', event => {
        if (event.target.matches('[data-scheduler] [data-color-anchor]') &&
            (event.key === 'Enter' || event.key === ' ')) {
            event.preventDefault();
            if (!event.target.disabled) event.target.click();
        }
    });
    document.addEventListener('click', event => {
        const button = event.target.closest('[data-scheduler] [data-action]');
        if (!button || button.disabled) return;
        const root = button.closest('[data-scheduler]');
        try {
            prepare(root);
            switch (button.dataset.action) {
                case 'save-scheduler': save(root); break;
                case 'reload-scheduler': reload(root); break;
                case 'add-scheduler-row': addRow(root); break;
                case 'remove-selected-scheduler-rows': removeSelectedRows(root); break;
            }
        } catch (error) {
            root.querySelector('[data-scheduler-status]').textContent = error.message;
        }
    });

    document.addEventListener('input', event => {
        const element = event.target.closest('[data-scheduler] tr[data-slot]');
        if (!element) return;
        if (event.target.dataset.field === 'value') updateValue(element);
        if (event.target.dataset.field === 'color') syncPicker(element);
    });
    document.addEventListener('change', event => {
        const element = event.target.closest('[data-scheduler] tr[data-slot]');
        if (!element) return;
        if (event.target.matches('[data-row-selected]')) updateActions(element.closest('[data-scheduler]'));
        if (event.target.dataset.field === 'range') updateValue(element, true);
        if (event.target.dataset.field === 'color') syncPicker(element);
    });
    // Observe only structural changes. Enhancement runs once per anchor, and never rewrites live outputs.
    const observer = new MutationObserver(refreshPickers);
    observer.observe(document.documentElement, { childList: true, subtree: true });
    document.addEventListener('load', refreshPickers, true);
    refreshPickers();
})();