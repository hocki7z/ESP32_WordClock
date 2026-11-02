(() => {
    function addFirmwareUploader() {
        const settings = document.getElementById('settings');
        if (!settings) return;

        const card = Array.from(settings.children).find(element => {
            if (!element.classList.contains('card')) return false;
            const heading = element.querySelector('h5');
            return heading && heading.textContent.trim() === 'Firmware Update';
        });
        if (!card || document.getElementById('wordclock-ota-uploader')) return;

        const panel = document.createElement('div');
        panel.id = 'wordclock-ota-uploader';
        panel.style.cssText = 'padding:12px 0;text-align:left';

        const picker = document.createElement('input');
        picker.type = 'file';
        picker.accept = '.bin,application/octet-stream';
        picker.hidden = true;

        const fileField = document.createElement('div');
        fileField.style.cssText = 'display:flex;flex-direction:row;align-items:center;justify-content:flex-start;text-align:left;color:black;background-color:#fff;width:100%;height:32px;margin:0 0 8px;padding:3px;gap:8px;box-sizing:border-box;border:1px solid #808080;border-radius:4px';
        const chooseFileButton = document.createElement('button');
        chooseFileButton.type = 'button';
        chooseFileButton.textContent = 'Choose firmware file';
        chooseFileButton.style.cssText = 'flex:0 0 auto;width:auto;height:24px;min-height:0;margin:0;padding:2px 8px;font-size:.85em;line-height:1.2;box-sizing:border-box;cursor:pointer';
        chooseFileButton.addEventListener('click', () => picker.click());

        const filename = document.createElement('span');
        filename.textContent = 'No file selected';
        filename.style.cssText = 'flex:0 1 auto;min-width:0;text-align:center;overflow-wrap:anywhere;font-weight:300;font-size:.9em';
        picker.addEventListener('change', () => {
            filename.textContent = picker.files && picker.files[0]
                ? picker.files[0].name
                : 'No file selected';
        });
        fileField.append(chooseFileButton, filename);

        const button = document.createElement('button');
        button.type = 'button';
        button.textContent = 'Upload and install';
        button.style.cssText = 'width:100%;padding:10px;cursor:pointer';

        const progress = document.createElement('progress');
        progress.max = 100;
        progress.value = 0;
        progress.hidden = true;
        progress.style.cssText = 'display:block;width:100%;margin:10px 0';

        const status = document.createElement('p');
        status.setAttribute('role', 'status');
        status.textContent = 'Choose a .bin image to begin.';

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