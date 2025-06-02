
# SSHeesh

## Introduction

Hello!
I've been interested in experimenting with malware development for a while, and now that I have some time, I'm finally diving in.

This project starts with the basics:

##  Stealing SSH Keys

The initial goal is simple: extract the victim's SSH keypairs and exfiltrate them. The keys are encrypted with AES and sent to a Command and Control (C2) server. The server then decrypts the data and makes the SSH credentials available.

## Browser Credential Theft

After getting SSH key exfiltration working, I started exploring credential theft from web browsers. This part proved more complex due to the diversity of browser architectures and credential storage methods.

Since I use **Zen** (a Firefox-based browser) daily, I began my research there. I discovered that Firefox-based browsers store credential data across three essential files:

* `cert9.db`
* `key4.db`
* `logins.json`

Rather than decrypting the credentials on the victim’s machine, I opted to send these files as-is through the C2 server. On the server side, I use [firefox\_decrypt](https://github.com/unode/firefox_decrypt) to extract the credentials, this tool works well and saves time on reimplementing decryption logic.

## 📌 TODO

* [ ] Improve the C2 communication (currently using a hardcoded static IP, which isn’t viable for real-world deployment)
* [ ] Add support for more browsers (Chromium-based browsers are not yet implemented)
* [ ] Extend the project to support Windows systems (which would be a good learning opportunity, since I’m currently unfamiliar with Windows internals)
---

**PS:** This tool was *obviously* developed for educational purposes only and will **never** be used to actually cause harm to anyone :)

---
