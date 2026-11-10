(() => {
    // Keep application refreshes separate from ESPUI's structural GUI transfer.
    let viewSocket = null;
    let guiComplete = false;
    let lastView = '';
    let lastViewSent = 0;
    function reportClientView() {
        if (typeof websock === 'undefined' || websock.readyState !== WebSocket.OPEN) return;
        if (viewSocket !== websock) {
            viewSocket = websock;
            guiComplete = false;
            lastView = '';
            // ESPUI changes the connection-status DOM on open, before GUI data.
            // Its own onmessage handler constructs the DOM before this listener.
            const socket = viewSocket;
            socket.addEventListener('message', event => {
                if (socket !== viewSocket) return;
                let packet;
                try { packet = JSON.parse(event.data); } catch (_) { return; }
                if (packet.type === UI_INITIAL_GUI) {
                    guiComplete = false;
                    lastView = '';
                }
                if (packet.type === UI_INITIAL_GUI || packet.type === UI_EXTEND_GUI) {
                    guiComplete = Array.isArray(packet.controls) &&
                        (packet.startindex || 0) + packet.controls.length - 1 >= packet.totalcontrols;
                }
                reportClientView();
            });
        }
        if (!guiComplete) return;
        // A final chunk may still contain an asynchronously assembled control.
        if (typeof controlAssemblyArray !== 'undefined' && controlAssemblyArray.some(Boolean)) return;

        const activeLink = document.querySelector('#tabsnav li.active a');
        const target = activeLink && activeLink.getAttribute('href');
        const panel = target && document.getElementById(target.slice(1));
        const tab = panel && panel.getAttribute('data-control-id');
        if (!tab || !/^\d+$/.test(tab)) return;

        const message = 'wcview:' + (document.hidden ? '0' : '1') + ':' + tab;
        const now = Date.now();
        if (message !== lastView || now - lastViewSent >= 2000) {
            websock.send(message);
            lastView = message;
            lastViewSent = now;
        }
    }

    // DOM observation covers default/hash tabs, keyboard and programmatic changes,
    // rebuilds and reconnects, not just ESPUI's mouse-up tab callback.
    const viewObserver = new MutationObserver(reportClientView);
    viewObserver.observe(document.documentElement, {
        childList: true, subtree: true, attributes: true, attributeFilter: ['class']
    });
    document.addEventListener('visibilitychange', reportClientView);
    setInterval(reportClientView, 2000); // Retry if the device's event queue was full.

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