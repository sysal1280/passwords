### Passwords

Managing passwords securely is critical, yet the majority of people still don’t use password managers. This leaves most users vulnerable to weak or reused passwords and the growing risk of breaches. While many commercial solutions exist, they often come with subscriptions, cloud storage requirements, or proprietary code. This project takes a different approach: a free, open‑source, GPG‑based password manager that works entirely offline and across platforms. By removing barriers like cost, internet dependency, and closed ecosystems, it aims to make strong password management accessible to everyone and encourage wider adoption of secure practices.

To achieve this, the project is built on a few core principles:

- Your passwords never leave your device. No internet connection is required, reducing exposure to online threats, and fitting into your Tier zero and Tier one environments seamlessly.
- The code is fully auditable. Anyone can inspect, verify, and contribute, ensuring trust through transparency.
- Free to use forever. You’re not tied to a company’s servers, business model or subscription.
- Cloud-based managers are attractive targets for hackers. By keeping everything local, this tool reduces risk.
- Works seamlessly across Linux, macOS, and Windows, so you can manage credentials wherever you are.



#### Features

- GPG-based encryption for maximum security and encryption configuration. 
  - Use supported devices like smartcards and yubikeys to store private keys, making you even safer.
  - Configure GPG to use the cipher algorithm and other settings *you* want to use.
- Cross-platform support (Linux, macOS, Windows).
  - 64-Bit operating system support.
- No proprietary dependencies. Passwords is fully open-source.



#### Installation on Linux and OSX

This program requires Qt.

To build and install this program:

```bash
git clone git@github.com:sysal1280/passwords.git
cd passwords/
mkdir build
cd build
cmake ../ -DCMAKE_PREFIX_PATH=/home/sysal/Qt/6.9.3/gcc_64 -DCMAKE_BUILD_TYPE=Release
make
```



#### Installation on Windows

Download the setup program from Releases.

Or install Qt Creator and download and build the software from source code yourself.



#### License

This program is licensed under **GNU GPL v3**. See LICENSE for details.



#### Screenshots



![image-20251212082424006](/home/sysal/.config/Typora/typora-user-images/image-20251212082424006.png)





