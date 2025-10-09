document.addEventListener('alpine:init', () => {
    console.log("On init");
    Alpine.data('ledData', () => ({
        busy: false,
        color: '#00ff00',
        currentColor: '#000000',
        msg: '',
        err: '',
        ok: '',
        async init() {
            console.log("ledData on Init");
            try {
                const r = await fetch('/api/led');
                if (r.ok) {
                    const data = await r.json();
                    this.currentColor = data.color || this.currentColor;
                    this.color = data.color || this.color;
                }                         
            } catch (_) { /* ignore on first load */ }
        },
        async applyColor() {
            this.busy = true;
            this.msg = '';
            this.err = '';

            try {
                const r = await fetch('/api/led', {
                    method: 'POST',
                    headers: {'Content-Type':'application/json'},
                    body: JSON.stringify({ color: this.color }) // "#RRGGBB"
                });
                if (!r.ok) throw new Error(await r.text());
                const data = await r.json();
                this.currentColor = data.color || this.color;
                this.msg = 'LED updated';
            } catch (e) {
                this.err = e.message || 'Failed to update LED';
            } finally {
                this.busy = false;
            }
        }
        
    }));

    Alpine.data('wifiSettings', () => ({
        wifiBusy: false,
        credentials : {
            ssid: '',
            pass: '',
            token: '',
        },
        err: '',
        ok: '',
        async init() {
            try {
                this.wifiBusy = true;
                const wifi = await fetch('/api/wifi');
                if (wifi.ok) {
                    const data = await wifi.json();
                    this.credentials = {
                        ssid: data.ssid,
                        pass: data.pass,
                        token: data.token
                    }
                }
            } catch (_) {
            } finally {
                this.wifiBusy = false;
            }
        },
        async saveWifiSettings() {
            this.wifiBusy = true;
            try {
                const r = await fetch('/api/wifi', {
                    method: 'POST',
                    headers: {'Content-Type':'application/json'},
                    body: JSON.stringify({
                        ssid: this.wifiSettings.ssid,
                        pass: this.wifiSettings.pass,
                        token: this.wifiSettings.token
                    }),
                });
                if (!r.ok) throw new Error(await r.text());
                //this.wifiMsg = 'Wifi Settings Upddated';
            } catch (e) {
                console.log("ERROR saving wifi")
                //this.wifiErr = e.message || 'Failed to wifi settings';
            } finally {
                this.wifiBusy = false;
                //this.wifiErr = false;
            }
        }
    }));
})

