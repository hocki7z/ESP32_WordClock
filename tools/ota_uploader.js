(() => {
    const text = {
        en: {
            groupTitle: 'Firmware Update',
            fileLabel: 'Firmware image (.bin)',
            chooseFile: 'Choose firmware file',
            noFile: 'No file selected',
            upload: 'Upload and install',
            initial: 'Choose a .bin image to begin.',
            invalidFile: 'Select a .bin application image.',
            starting: 'Starting firmware upload…',
            uploading: percent => 'Uploading ' + percent + '%',
            validating: 'Upload complete. Validating firmware…',
            connectionLost: 'Connection lost. The device may be restarting; wait and reconnect.',
            uploadFailed: status => 'Update failed (' + status + ').',
            success: 'Firmware accepted. The device is restarting now.',
            failure: 'Firmware update failed.'
        },
        de: {
            groupTitle: 'Firmware-Update',
            fileLabel: 'Firmwaredatei (.bin)',
            chooseFile: 'Firmwaredatei auswählen',
            noFile: 'Keine Datei ausgewählt',
            upload: 'Hochladen und installieren',
            initial: 'Wählen Sie eine .bin-Datei aus.',
            invalidFile: 'Bitte wählen Sie ein .bin-Firmwareabbild aus.',
            starting: 'Firmware-Upload wird gestartet…',
            uploading: percent => 'Upload: ' + percent + '%',
            validating: 'Upload abgeschlossen. Firmware wird geprüft…',
            connectionLost: 'Verbindung unterbrochen. Das Gerät startet möglicherweise neu; bitte warten und erneut verbinden.',
            uploadFailed: status => 'Update fehlgeschlagen (' + status + ').',
            success: 'Firmware akzeptiert. Das Gerät wird neu gestartet.',
            failure: 'Firmware-Update fehlgeschlagen.'
        }
    };

    function normalizeLanguage(language) {
        return String(language || '').toLowerCase().startsWith('de') ? 'de' : 'en';
    }

    let startupLanguage = window.wordclockUiLanguage || 'en';
    try {
        startupLanguage = window.wordclockUiLanguage || localStorage.getItem('wordclock-ui-language') || 'en';
    } catch (error) {
        // Continue in English when storage is unavailable.
    }
    let language = normalizeLanguage(startupLanguage);
    let uploaderElements = null;

    function translatedServerMessage(message, strings) {
        if (language !== 'de') return message;

        const exactMessages = {
            'Firmware accepted. The device is restarting now.': strings.success,
            'OTA upload disconnected.': 'Upload unterbrochen.',
            'Invalid firmware upload.': 'Ungültiger Firmware-Upload.',
            'Firmware upload data was incomplete or out of order.': 'Firmwaredaten sind unvollständig oder in falscher Reihenfolge eingegangen.',
            'The uploaded file is not a valid ESP32 application image.': 'Die hochgeladene Datei ist kein gültiges ESP32-Firmwareabbild.',
            'No valid embedded ProductInfo record was found in the firmware image.': 'Im Firmwareabbild wurde kein gültiger ProductInfo-Datensatz gefunden.',
            'Update refused: the selected firmware version is invalid (255.255.255).': 'Update abgelehnt: Die ausgewählte Firmwareversion ist ungültig (255.255.255).',
            'No firmware image was received or another upload is active.': 'Kein Firmwareabbild empfangen oder ein anderer Upload läuft bereits.'
        };
        if (exactMessages[message]) return exactMessages[message];

        const versionMatch = message.match(/^Update refused: firmware version (.+) is not newer than installed version (.+)\.$/);
        if (versionMatch) {
            return 'Update abgelehnt: Firmwareversion ' + versionMatch[1] +
                ' ist nicht neuer als die installierte Version ' + versionMatch[2] + '.';
        }
        if (/^Firmware update failed \(ESP error \d+\)\.$/.test(message)) {
            return message.replace('Firmware update failed', 'Firmware-Update fehlgeschlagen');
        }
        return message;
    }

    function renderStatus() {
        if (!uploaderElements) return;
        const strings = text[language];
        const { status, statusKey } = uploaderElements;
        const statusValue = uploaderElements.statusValue || {};
        const messages = {
            initial: strings.initial,
            invalidFile: strings.invalidFile,
            starting: strings.starting,
            uploading: strings.uploading(uploaderElements.progress.value),
            validating: strings.validating,
            connectionLost: strings.connectionLost,
            response: translatedServerMessage(statusValue.message, strings) || strings.uploadFailed(statusValue.status)
        };
        status.textContent = messages[statusKey] || strings.initial;
    }

    function setStatus(key, value) {
        if (!uploaderElements) return;
        uploaderElements.statusKey = key;
        uploaderElements.statusValue = value;
        renderStatus();
    }

    function applyTranslations() {
        if (!uploaderElements) return;
        const strings = text[language];
        uploaderElements.heading.textContent = strings.groupTitle;
        uploaderElements.fileCaption.textContent = strings.fileLabel;
        uploaderElements.chooseFileButton.textContent = strings.chooseFile;
        uploaderElements.filename.textContent = uploaderElements.picker.files && uploaderElements.picker.files[0]
            ? uploaderElements.picker.files[0].name
            : strings.noFile;
        uploaderElements.button.textContent = strings.upload;
        renderStatus();
    }

    // The application can set this before custom.js loads, or update it later
    // using setWordclockUiLanguage('en'|'de'). English is the default.
    window.setWordclockUiLanguage = function (newLanguage) {
        language = normalizeLanguage(newLanguage);
        try {
            localStorage.setItem('wordclock-ui-language', language);
        } catch (error) {
            // Language changes still apply for this page when storage is unavailable.
        }
        applyTranslations();
    };
    window.addEventListener('wordclock-language-change', event => {
        if (event.detail) window.setWordclockUiLanguage(event.detail);
    });

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
        picker.setAttribute('aria-label', 'Firmware image');

        const fileField = document.createElement('div');
        fileField.style.cssText = 'display:flex;flex-direction:row;align-items:center;justify-content:flex-start;text-align:left;color:black;background: rgba(255, 255, 255, 0.8);box-shadow: inset 0 0 6px rgba(0, 0, 0, 0.3);width:calc(100% - 4px);height:36px;margin:0 0 8px 2px;padding:6px;gap:8px;box-sizing:border-box;border:1px solid #808080;border-radius:4px';
        const chooseFileButton = document.createElement('button');
        chooseFileButton.type = 'button';
        chooseFileButton.style.cssText = 'flex:0 0 auto;width:auto;height:28px;min-height:0;margin: 3px 3px;padding:2px 8px;font-size:.85em;line-height:1.2;box-sizing:border-box;cursor:pointer';
        chooseFileButton.addEventListener('click', () => picker.click());

        const filename = document.createElement('span');
        filename.style.cssText = 'flex:0 1 auto;min-width:0;text-align:center;overflow-wrap:anywhere;font-weight:300;font-size:1em';
        picker.addEventListener('change', () => {
            filename.textContent = picker.files && picker.files[0]
                ? picker.files[0].name
                : text[language].noFile;
        });
        fileField.append(chooseFileButton, filename);

        const fileLabel = document.createElement('div');
        fileLabel.style.cssText = 'display:block;width:100%;font-weight:300;margin-bottom:6px;';
        const fileCaption = document.createElement('span');
        fileLabel.append(fileCaption);

        const button = document.createElement('button');
        button.type = 'button';
        button.style.cssText = 'width:100%;margin:3px 0;box-sizing:border-box;cursor:pointer';

        const progress = document.createElement('progress');
        progress.max = 100;
        progress.value = 0;
        progress.hidden = true;
        progress.style.cssText = 'display:block;width:calc(100% - 2px);height:18px;margin:8px 0 8px;box-sizing:border-box';

        const status = document.createElement('p');
        status.setAttribute('role', 'status');
        status.style.cssText = 'background-color:unset;text-align:left;padding:0;margin:0 0 8px 0px;top:0;font-weight:300;font-size:.9em;width:calc(100% - 4px);box-sizing:border-box';

        uploaderElements = {
            heading, fileCaption, chooseFileButton, filename, picker, button, progress, status,
            statusKey: 'initial', statusValue: ''
        };
        applyTranslations();

        button.addEventListener('click', () => {
            const file = picker.files && picker.files[0];
            if (!file || !file.name.toLowerCase().endsWith('.bin')) {
                setStatus('invalidFile');
                return;
            }

            button.disabled = true;
            picker.disabled = true;
            chooseFileButton.disabled = true;
            setStatus('starting');
            progress.hidden = false;
            progress.value = 0;

            const form = new FormData();
            form.append('firmware', file, file.name);
            const request = new XMLHttpRequest();
            request.open('POST', '/update');
            request.upload.onprogress = event => {
                if (event.lengthComputable) {
                    progress.value = Math.round(event.loaded * 100 / event.total);
                    setStatus('uploading');
                }
            };
            request.upload.onload = () => {
                progress.value = 100;
                setStatus('validating');
            };
            request.onload = () => {
                setStatus('response', { message: request.responseText || '', status: request.status });
                if (request.status !== 200) {
                    button.disabled = false;
                    picker.disabled = false;
                    chooseFileButton.disabled = false;
                }
            };
            request.onerror = () => {
                setStatus('connectionLost');
                button.disabled = false;
                picker.disabled = false;
                chooseFileButton.disabled = false;
            };
            request.send(form);
        });

        panel.append(fileLabel, fileField, picker, button, progress, status);
        card.appendChild(panel);
    }

    const observer = new MutationObserver(addFirmwareUploader);
    observer.observe(document.documentElement, { childList: true, subtree: true });
    addFirmwareUploader();
})();