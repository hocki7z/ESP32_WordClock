# Deferred Web UI updates

The initial ESPUI control structure is still sent in full. Expensive dynamic
refreshes (preview SVG, system-information HTML and WiFi scan results) start only
when a connected browser needs them.

- Preview frame events set one pending flag; intermediate frames are not stored.
  A refresh reads the latest shared display-scene snapshot.
- System information becomes pending on the 15-second timer and on changes in
  client visibility. Existing value caches still avoid unchanged label updates.
- WiFi scan completion marks WiFi controls pending. Merely switching tabs does
  not reset SSID/password selections or interrupt editing.
- Pending flags survive disconnects, hidden tabs and OTA suppression. Returning
  to the relevant tab, or ending a failed/cancelled OTA attempt, drains eligible
  pending work. Successful OTA normally restarts the device.
- Eligibility follows the full control-parent chain. A control outside all tabs
  is eligible whenever a client is connected.

## Browser and task coordination

The existing custom JavaScript bundle also reports the selected ESPUI tab and
document visibility over the current WebSocket (`wcview:<visible>:<tab-id>`).
Reporting waits for the current connection's complete GUI transfer and outstanding
control fragments. DOM observation covers default/hash selection, mouse, keyboard,
programmatic changes and rebuilds. Visibility changes are reported immediately;
a two-second heartbeat also retries the latest view.

The WebSocket observer forwards all ESPUI protocol events unchanged. It maintains
a mutex-protected application registry using the actual server-assigned client
IDs. It never traverses AsyncWebSocket's client container from the WebSite task.
Task notifications request reevaluation; only the WebSite task owns pending flags
and performs expensive rendering. OTA enable/disable state is atomic.

## Multiple clients and scope

Demand is the **union of visible tabs across clients**. If one browser views
Overview and another views Settings, both sets of pending controls can refresh.
Background documents do not demand tab-specific refreshes.

ESPUI has shared control values and broadcasts their changes. Therefore, a
refresh demanded by one client may still be transmitted to other clients viewing
different tabs. This feature suppresses unnecessary application work; it does
not implement per-client transport filtering, lazy initial GUI construction or
per-client WiFi editing state. Explicit user-action feedback remains immediate.

Strict per-client filtering would need ESPUI changes, including tab-entry catch-up
independent of its global control-change watermark.

## Validation

Open [the browser regression harness](../test/web_ui_visibility.html) to exercise
the production visibility reporter without a board or network connection.

On hardware, verify no preview/system rendering without clients, tab switching,
two clients on different tabs, background/foreground transitions, reconnects,
WiFi scan completion while Settings is hidden, and OTA failure/resume. A normal
PlatformIO firmware build regenerates the embedded custom JavaScript automatically.