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

        const description = document.createElement('p');
        description.textContent = 'Select the versioned <project>_ota_<major>_<minor>_<build>.bin file from your PlatformIO build output.';
        const picker = document.createElement('input');
        picker.type = 'file';
        picker.accept = '.bin,application/octet-stream';
        picker.style.cssText = 'display:block;width:100%;margin:8px 0';

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

        const onlineStatus = document.createElement('p');
        onlineStatus.setAttribute('role', 'status');
        onlineStatus.textContent = 'Online update status: not checked.';

        const checkButton = document.createElement('button');
        checkButton.type = 'button';
        checkButton.textContent = 'Check for update';
        checkButton.style.cssText = 'width:100%;padding:10px;margin:6px 0;cursor:pointer';

        const installButton = document.createElement('button');
        installButton.type = 'button';
        installButton.textContent = 'Start update now';
        installButton.disabled = true;
        installButton.style.cssText = 'width:100%;padding:10px;margin:6px 0;cursor:pointer';

        const scheduleLabel = document.createElement('label');
        scheduleLabel.textContent = 'Automatic update checks (web UI must remain open)';
        scheduleLabel.style.cssText = 'display:block;margin-top:12px';
        const schedule = document.createElement('select');
        schedule.style.cssText = 'display:block;width:100%;margin:6px 0';
        [
            ['off', 'Not automatic'],
            ['after5', '5 minutes after web UI starts'],
            ['hourly', 'Every hour, on the hour'],
        ].forEach(([value, label]) => {
            const option = document.createElement('option');
            option.value = value;
            option.textContent = label;
            schedule.appendChild(option);
        });
        const savedSchedule = localStorage.getItem('wordclock-update-check-schedule');
        schedule.value = ['off', 'after5', 'hourly'].includes(savedSchedule) ? savedSchedule : 'off';

        const manifestUrl = 'https://raw.githubusercontent.com/hocki7z/ESP32_WordClock/fw_updates/latest.json';
        let availableFirmware = null;
        let installedVersion = null;
        let dailyCheckKey = '';
        let scheduledTimeout = 0;
        let scheduleInterval = 0;

        const parseVersion = value => {
            if (typeof value !== 'string' || !/^\d{1,3}\.\d{1,3}\.\d{1,3}$/.test(value)) {
                throw new Error('Update manifest has an invalid version.');
            }
            const version = value.split('.').map(Number);
            if (version.some(part => part > 255)) throw new Error('Update version components must be 0–255.');
            return version;
        };
        const compareVersions = (left, right) => {
            for (let index = 0; index < 3; index++) {
                if (left[index] !== right[index]) return left[index] > right[index] ? 1 : -1;
            }
            return 0;
        };
        const versionText = version => version.join('.');

        const checkForUpdate = async () => {
            if (button.disabled) return;
            checkButton.disabled = true;
            installButton.disabled = true;
            availableFirmware = null;
            onlineStatus.textContent = 'Checking for firmware updates…';
            try {
                const [manifestResponse, currentResponse] = await Promise.all([
                    fetch(manifestUrl, { cache: 'no-store' }),
                    fetch('/firmware-version', { cache: 'no-store' }),
                ]);
                if (!manifestResponse.ok) throw new Error('Update manifest request failed (' + manifestResponse.status + ').');
                if (!currentResponse.ok) throw new Error('Could not read the installed firmware version.');

                const manifest = await manifestResponse.json();
                const current = await currentResponse.json();
                const latestVersion = parseVersion(manifest.version);
                if (typeof manifest.url !== 'string') throw new Error('Update manifest is missing its firmware URL.');
                const firmwareUrl = new URL(manifest.url, manifestUrl);
                if (firmwareUrl.protocol !== 'https:' || firmwareUrl.hostname !== 'raw.githubusercontent.com' ||
                    !firmwareUrl.pathname.startsWith('/hocki7z/ESP32_WordClock/fw_updates/')) {
                    throw new Error('Firmware URL must be an HTTPS file from the fw_updates branch.');
                }

                installedVersion = current.valid ? current.version : null;
                if (!installedVersion || compareVersions(latestVersion, installedVersion) > 0) {
                    availableFirmware = { version: latestVersion, url: firmwareUrl.href };
                    onlineStatus.textContent = 'A new update is available: v' + versionText(latestVersion) +
                        (installedVersion ? ' (installed v' + versionText(installedVersion) + ').' : '.');
                    installButton.disabled = false;
                } else {
                    onlineStatus.textContent = compareVersions(latestVersion, installedVersion) === 0
                        ? 'Firmware is up to date (v' + versionText(installedVersion) + ').'
                        : 'Installed firmware is newer than the online version.';
                }
            } catch (error) {
                onlineStatus.textContent = 'Update check failed: ' + (error.message || 'network error.');
            } finally {
                checkButton.disabled = false;
            }
        };

        const uploadFirmware = file => {
            button.disabled = true;
            picker.disabled = true;
            checkButton.disabled = true;
            installButton.disabled = true;
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
                    checkButton.disabled = false;
                    installButton.disabled = !availableFirmware;
                }
            };
            request.onerror = () => {
                status.textContent = 'Connection lost. The device may be restarting; wait and reconnect.';
                button.disabled = false;
                picker.disabled = false;
                checkButton.disabled = false;
                installButton.disabled = !availableFirmware;
            };
            request.send(form);
        };

        checkButton.addEventListener('click', checkForUpdate);
        installButton.addEventListener('click', async () => {
            if (!availableFirmware) return;
            installButton.disabled = true;
            onlineStatus.textContent = 'Downloading firmware v' + versionText(availableFirmware.version) + '…';
            try {
                const response = await fetch(availableFirmware.url, { cache: 'no-store' });
                if (!response.ok) throw new Error('Firmware download failed (' + response.status + ').');
                const blob = await response.blob();
                if (!blob.size) throw new Error('Downloaded firmware file is empty.');
                const filename = availableFirmware.url.split('/').pop() || 'firmware.bin';
                if (!filename.toLowerCase().endsWith('.bin')) throw new Error('Manifest firmware URL must point to a .bin file.');
                onlineStatus.textContent = 'Download complete. Installing firmware…';
                uploadFirmware(new File([blob], filename, { type: 'application/octet-stream' }));
            } catch (error) {
                onlineStatus.textContent = 'Update download failed: ' + (error.message || 'network error.');
                installButton.disabled = false;
            }
        });

        schedule.addEventListener('change', () => {
            localStorage.setItem('wordclock-update-check-schedule', schedule.value);
            onlineStatus.textContent = 'Automatic checking: ' + schedule.options[schedule.selectedIndex].text + '.';
            runScheduledCheck();
        });

        const runScheduledCheck = () => {
            window.clearTimeout(scheduledTimeout);
            window.clearInterval(scheduleInterval);
            scheduledTimeout = 0;
            scheduleInterval = 0;
            if (schedule.value === 'after5') {
                scheduledTimeout = window.setTimeout(() => {
                    if (schedule.value === 'after5') checkForUpdate();
                }, 5 * 60 * 1000);
            } else if (schedule.value === 'hourly') {
                scheduleInterval = window.setInterval(() => {
                    const now = new Date();
                    const key = now.getFullYear() + '-' + now.getMonth() + '-' + now.getDate() + '-' + now.getHours();
                    if (now.getMinutes() === 0 && key !== dailyCheckKey) {
                        dailyCheckKey = key;
                        checkForUpdate();
                    }
                }, 30 * 1000);
            }
        };
        runScheduledCheck();

        button.addEventListener('click', () => {
            const file = picker.files && picker.files[0];
            if (!file || !file.name.toLowerCase().endsWith('.bin')) {
                status.textContent = 'Select a .bin application image.';
                return;
            }

            uploadFirmware(file);
        });

        panel.append(description, picker, button, progress, status,
            document.createElement('hr'), onlineStatus, checkButton, installButton, scheduleLabel, schedule);
        card.appendChild(panel);
    }

    const observer = new MutationObserver(addFirmwareUploader);
    observer.observe(document.documentElement, { childList: true, subtree: true });
    addFirmwareUploader();
})();
