Got it ✅ Here’s a clean **`/tests/api_smoke.md`** file in English with 5 smoke test points.

````markdown
# API Smoke Tests (M5Stack Atom Lite — AP mode)

**Target**
- Wi-Fi: `ATOM_LED_AP` / `atom1234`
- Base URL: `http://192.168.4.1`

---

## 1. GET /get
```bash
curl -s http://192.168.4.1/get
````

**Expect:** plain text string in the exact format `#RRGGBB`
Example: `#00FF00`

---

## 2. GET /set (valid value)

```bash
curl -s "http://192.168.4.1/set?value=%23FF0000"
curl -s http://192.168.4.1/get
```

**Expect:** first call returns `OK`, second call returns `#FF0000`.
LED should turn red.

---

## 3. GET /set (invalid format)

```bash
curl -s -i "http://192.168.4.1/set?value=red"
```

**Expect:** HTTP 400 Bad Request
Response body: `Bad format. Use #RRGGBB`
LED color should remain unchanged.

---

## 4. Idempotency check

```bash
curl -s "http://192.168.4.1/set?value=%2300FF00"
curl -s "http://192.168.4.1/set?value=%2300FF00"
```

**Expect:** both calls succeed with `OK`.
Device should not hang when applying the same value repeatedly.

---

## 5. Unknown route

```bash
curl -s -i http://192.168.4.1/unknown
```

**Expect:** HTTP 404 Not Found
Response body: `Not found`

```

