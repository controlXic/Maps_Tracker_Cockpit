# Development Notes and System Configuration

## 1\. Convert OSM PBF to MBTiles

Convert an OSM PBF file into an MBTiles database using Tilemaker.

bash

```bash
tilemaker \
  --input ./data/spain.osm.pbf \
  --output ./data/spain.mbtiles \
  --config /path/to/tilemaker/resources/config-openmaptiles.json \
  --process /path/to/tilemaker/resources/process-openmaptiles-offroad.lua
```

## 2\. Configure Map Styles in QGIS

- Open the generated MBTiles file in QGIS.
- Import the downloaded map style from MapTiler.
- Create separate GPX styles for each feature type:
  - Tracks
  - Routes
  - Waypoints

## 3\. Extract Cities and Towns from OSM Data

Install Osmium Tool:

bash

```bash
sudo apt install osmium-tool
```

Extract cities, towns, and villages from the OSM PBF file:

bash

```bash
osmium tags-filter spain.osm.pbf \
  n/place=city \
  n/place=town \
  n/place=village \
  -o places.osm.pbf
```

Export the extracted data to GeoJSON:

bash

```bash
osmium export places.osm.pbf -o places.geojson
```

## 4\. Configure Geographic Layers in QGIS

- Open the GeoJSON files in QGIS.
- Load city and town name layers, as well as any other required geographic layers.
- Convert the GeoJSON files to GeoPackage (GPKG).
- Configure the layer settings, including layer names, colors, and styles.

## 5\. Install Qt Serial Port on Orange Pi

Install the Qt Serial Port libraries:

bash

```bash
sudo apt-get install libqt5serialport5
sudo apt-get install libqt5serialport5-dev
```

## 6\. Configure Serial Port Permissions

Add the current user to the `dialout` group:

bash

```bash
sudo usermod -a -G dialout $USER
groups
```

List available serial ports:

bash

```bash
ls /dev/ttyS*
```

Example output:

text

```text
/dev/ttyS0  /dev/ttyS1  /dev/ttyS2
/dev/ttyS3  /dev/ttyS4  /dev/ttyS5
```

Test the serial port:

bash

```bash
gpio serial /dev/ttyS5
```

**Note:** Log out and log back in for the new group permissions to take effect.

## 7\. Configure the Keyboard on Orange Pi

- Configure the physical keyboard using `orangepi-config`.
- Configure the virtual keyboard separately.

## 8\. Configure the Boot Splash Screen

Configure the boot splash screen using the Armbian boot logo configuration.

### Disable the current splash screen

bash

```bash
sudo cp watermark.png /usr/share/plymouth/themes/orangepi/
```

### Install the custom boot splash screen

bash

```bash
sudo cp bootsplash.orangepi /usr/lib/firmware/
```

### Update the initramfs and reboot

bash

```bash
sudo update-initramfs -v -u
sudo reboot
```

## 9\. Install a Minimal QGIS Environment

Check the required shared libraries:

bash

```bash
ldd ./c_x_maps_qgis
```

Update the package list:

bash

```bash
sudo apt update
```

Install the minimum required QGIS and Qt dependencies:

bash

```bash
sudo apt install --no-install-recommends \
  libqgis-core3.22.16 \
  libqgis-gui3.22.16 \
  libqt5charts5 \
  libqt5serialport5
```

Set the QGIS installation path:

bash

```bash
export QGIS_PREFIX_PATH=/usr
```

## 10\. Network Configuration (Wi-Fi / Access Point)

Restart the network services:

bash

```bash
sudo systemctl restart NetworkManager
sudo systemctl restart wpa_supplicant
```

Restart the wireless interface:

bash

```bash
sudo ip link set wlan0 down
sudo ip link set wlan0 up
sudo systemctl restart NetworkManager
```

Scan for available Wi-Fi networks:

bash

```bash
sudo iw dev wlan0 scan | grep SSID
```

Connect to a Wi-Fi network:

bash

```bash
sudo wpa_passphrase "YOUR_SSID" "YOUR_PASSWORD" > wifi.conf
sudo wpa_supplicant -B -i wlan0 -c wifi.conf
sudo dhclient wlan0
```

### NetworkManager configuration

Edit the NetworkManager configuration file:

bash

```bash
sudo nano /etc/NetworkManager/NetworkManager.conf
```

Check the `unmanaged-devices` setting.

Restart NetworkManager:

bash

```bash
sudo systemctl restart NetworkManager
```

## 11\. Configure Application Sudo Permissions

Edit the sudoers configuration:

bash

```bash
sudo visudo
```

Add the following line at the end of the file, replacing the username and application path with the appropriate values:

text

```text
USERNAME ALL=(ALL) NOPASSWD: /path/to/application
```

This allows the specified user to execute the application with sudo without entering a password.

## 12\. Connect to the Orange Pi via SSH

Connect to the Orange Pi remotely:

bash

```bash
ssh USERNAME@DEVICE_IP
```

Navigate to the application directory:

bash

```bash
cd /path/to/application
```

Make the executable file executable:

bash

```bash
chmod +x c_x_maps_qgis
```

## 13\. Configure GitLab SSH Authentication

Generate an Ed25519 SSH key pair for GitLab authentication:

bash

```bash
ssh-keygen -t ed25519 -C "your_email@example.com"
```

Display the public key:

bash

```bash
cat ~/.ssh/id_ed25519.pub
```

Add the contents of the public key to your GitLab account under **Preferences → SSH Keys**.

**Security note:** Never publish private SSH keys (`id_ed25519`) in your repository. Only share the public key (`id_ed25519.pub`).

## 14\. Enable UART2 Device Tree Overlay

Enable the UART2 device tree overlay to configure the serial interface used by the relay pull-up circuit.

In the Armbian configuration, enable:

text

```text
[*] pi-uart2
```

Save the configuration and reboot if required.
