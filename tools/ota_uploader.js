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
        const state = pickers.get(anchor);
        if (!state || !state.ready) return;
        const color = element.querySelector('[data-field="color"]').value;
        state.syncing = true;
        try {
            state.picker.setColor(color, true);
            state.picker.applyColor(true);
            if (anchor.style.backgroundColor !== color) anchor.style.backgroundColor = color;
        } finally {
            state.syncing = false;
        }
    }

    // Use the bundled Pickr API (https://github.com/simonwep/pickr); keep anchors stable.
    function refreshPickers() {
        for (const [anchor, state] of pickers) {
            if (!anchor.isConnected) {
                state.ready = false;
                state.picker.destroyAndRemove();
                pickers.delete(anchor);
            }
        }
        clearTimeout(pickerRetry);
        pickerRetry = undefined;
        const available = window.Pickr && typeof window.Pickr.create === 'function';
        let waitingForLibrary = false;
        document.querySelectorAll('[data-scheduler] [data-color-anchor]').forEach(anchor => {
            if (pickers.has(anchor)) return;
            const element = anchor.closest('tr[data-slot]');
            const input = element && element.querySelector('[data-field="color"]');
            if (!input) return;
            // ESPUI replaces label HTML; clones may retain a previous enhancement's styles.
            // Explicit display also wins over author CSS that overrides the hidden attribute.
            anchor.hidden = true;
            anchor.style.display = 'none';
            input.hidden = false;
            input.style.display = 'block';
            input.disabled = false;
            if (!available) { waitingForLibrary = true; return; }
            const state = { picker: null, syncing: false, ready: false, open: false };
            try {
                state.picker = window.Pickr.create({
                    el: anchor, useAsButton: true, theme: 'monolith', default: input.value,
                    lockOpacity: true, defaultRepresentation: 'HEX',
                    swatches, components: { preview: true, opacity: false, hue: true,
                        interaction: { hex: true, input: true, save: true, cancel: true } }
                });
            } catch (error) {
                return; // Leave the native input visible if enhancement is unavailable.
            }
            pickers.set(anchor, state);
            state.picker.on('init', () => {
                if (pickers.get(anchor) !== state) return;
                state.ready = true;
                syncPicker(element);
                // Hand off only after init: the browser's native picker must no longer be reachable.
                input.hidden = true;
                input.style.display = 'none';
                input.disabled = true;
                anchor.textContent = 'Color';
                anchor.hidden = false;
                anchor.style.display = 'inline-block';
            }).on('show', () => {
                if (!state.ready) { state.picker.hide(); return; }
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
        if (!root) throw new Error('Scheduler table not available.');
        const elements = Array.from(root.querySelectorAll('tbody tr[data-slot]'));
        if (elements.length !== slotCount || elements.some((element, index) => Number(element.dataset.slot) !== index)) {
            throw new Error('Expected eight unique scheduler slots.');
        }
        return elements;
    }

    function getRows(root = document.querySelector('[data-scheduler]')) {
        return getElements(root).map(getRow);
    }

    // Validate the complete batch before applying any row, avoiding partial updates on bad data.
    function setRows(rows, root = document.querySelector('[data-scheduler]')) {
        const elements = getElements(root);
        if (!Array.isArray(rows) || rows.length !== slotCount) throw new Error('Expected eight scheduler rows.');
        const ordered = new Array(slotCount);
        for (const row of rows) {
            validateRow(row);
            if (ordered[row.slot]) throw new Error('Duplicate scheduler slot.');
            ordered[row.slot] = row;
        }
        for (const element of elements) {
            for (const name of fields) {
                if (!element.querySelector('[data-field="' + name + '"]')) throw new Error('Incomplete scheduler row.');
            }
        }
        elements.forEach((element, index) => setRow(element, ordered[index]));
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
            status.textContent = 'Submitting all slots…';
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
        }
    }

    window.WordClockScheduler = { getRow, setRow, getRows, setRows, save };

    // Delegation survives ESPUI replacing the label or loading it after this script runs.
    document.addEventListener('click', event => {
        const button = event.target.closest('[data-action="save-scheduler"]');
        if (button) save(button.closest('[data-scheduler]'));
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
        if (event.target.dataset.field === 'range') updateValue(element, true);
        if (event.target.dataset.field === 'color') syncPicker(element);
    });
    // Observe only structural changes. Enhancement runs once per anchor, and never rewrites live outputs.
    const observer = new MutationObserver(refreshPickers);
    observer.observe(document.documentElement, { childList: true, subtree: true });
    document.addEventListener('load', refreshPickers, true);
    refreshPickers();
})();