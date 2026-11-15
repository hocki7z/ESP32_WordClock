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
    const ranges = [[0, 100], [20, 50]];
    const fields = ['active', 'on', 'from', 'to', 'time', 'color', 'message', 'range', 'value'];
    const pickers = new Map();

    // Keep native controls as the source of truth; Pickr is a progressive enhancement.
    function syncRow(element) {
        const field = name => element.querySelector('[data-field="' + name + '"]');
        const bounds = ranges[Number(field('range').value)];
        const slider = field('value');
        if (bounds) {
            slider.min = bounds[0];
            slider.max = bounds[1];
        }
        const output = element.querySelector('[data-value-output]');
        if (output.textContent !== slider.value) output.textContent = slider.value;
        const color = field('color').value.toUpperCase();
        const button = element.querySelector('[data-color-picker]');
        if (button) {
            if (button.textContent !== color) button.textContent = color;
            button.style.background = color;
            button.style.color = parseInt(color.slice(1), 16) > 0x7FFFFF ? '#111' : '#fff';
        }
        const entry = pickers.get(element);
        if (entry && !entry.syncing) {
            entry.baseline = color;
            entry.syncing = true;
            entry.picker.setColor(color, true);
            entry.picker.applyColor(true);
            entry.syncing = false;
        }
    }

    // ESPUI can replace labels, so dispose detached popups and initialize new anchors once.
    function initControls() {
        for (const [element, entry] of pickers) {
            if (!element.isConnected) {
                entry.picker.destroyAndRemove();
                pickers.delete(element);
            }
        }
        for (const element of document.querySelectorAll('[data-scheduler] tr[data-slot]')) {
            const button = element.querySelector('[data-color-picker]');
            if (!button || pickers.has(element)) continue;
            syncRow(element);
            if (!window.Pickr) continue;
            const color = element.querySelector('[data-field="color"]');
            const entry = { picker: null, syncing: false, ready: false, open: false, baseline: color.value };
            entry.picker = window.Pickr.create({
                el: button, useAsButton: true, theme: 'monolith', default: color.value,
                lockOpacity: true, defaultRepresentation: 'HEX', comparison: true,
                swatches: [
                    '#FFFFFF', '#000000', '#FF0000', '#FF8000',
                    '#FFFF00', '#00FF00', '#00FFFF', '#0000FF',
                    '#8000FF', '#FF00FF', '#FF80C0', '#FFAA00'
                ],
                components: { preview: true, hue: true, opacity: false,
                    interaction: { input: true, save: true, cancel: true, clear: false } }
            });
            pickers.set(element, entry);
            entry.picker.on('init', () => { entry.ready = true; syncRow(element); });
            entry.picker.on('show', () => {
                entry.baseline = color.value;
                entry.open = true;
            });
            entry.picker.on('save', selected => {
                if (!entry.ready || !entry.open || entry.syncing || !selected) return;
                color.value = selected.toHEXA().toString().slice(0, 7);
                entry.baseline = color.value;
                syncRow(element);
                entry.picker.hide();
            });
            entry.picker.on('cancel', () => entry.picker.hide());
            entry.picker.on('hide', () => {
                entry.open = false;
                color.value = entry.baseline;
                syncRow(element);
            });
            color.hidden = true;
            button.hidden = false;
        }
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
        if (typeof row.color !== 'string' || !/^#[0-9a-f]{6}$/i.test(row.color)) throw new Error('Invalid color.');
        if (typeof row.message !== 'string' || new TextEncoder().encode(row.message).length > 64 || /[\x00-\x1f\x7f]/.test(row.message)) {
            throw new Error('Message must be a single line of at most 64 UTF-8 bytes.');
        }
        const bounds = ranges[row.range];
        if (!Number.isInteger(row.range) || !bounds || !Number.isInteger(row.value) || row.value < bounds[0] || row.value > bounds[1]) {
            throw new Error('Value is outside the selected range.');
        }
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
        // Apply range bounds before assigning the value, avoiding the browser's automatic clamping.
        const slider = element.querySelector('[data-field="value"]');
        slider.min = ranges[row.range][0];
        slider.max = ranges[row.range][1];
        slider.value = row.value;
        syncRow(element);
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

    document.addEventListener('input', event => {
        const element = event.target.closest('[data-scheduler] tr[data-slot]');
        if (element && ['range', 'value', 'color'].includes(event.target.dataset.field)) syncRow(element);
    });
    const schedulerObserver = new MutationObserver(initControls);
    schedulerObserver.observe(document.documentElement, { childList: true, subtree: true });
    initControls();

    // Delegation survives ESPUI replacing the label or loading it after this script runs.
    document.addEventListener('click', event => {
        const button = event.target.closest('[data-action="save-scheduler"]');
        if (button) save(button.closest('[data-scheduler]'));
    });
})();