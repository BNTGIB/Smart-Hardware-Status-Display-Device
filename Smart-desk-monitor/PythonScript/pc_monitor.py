import socket
import time
import psutil
import pynvml

ESP_HOST = "smartmonitor.local"
ESP_PORT = 4210
INTERVAL = 1

def resolve_esp_ip(hostname):
    try:
        return socket.gethostbyname(hostname)
    except socket.gaierror:
        return None

def get_gpu_stats(handle):
    """Đọc tải và nhiệt độ GPU, trả về (0, 0) nếu card rời đang tắt nguồn (Optimus D3Cold)"""
    if handle is None:
        return 0, 0
    try:
        util = pynvml.nvmlDeviceGetUtilizationRates(handle).gpu
        temp = pynvml.nvmlDeviceGetTemperature(handle, pynvml.NVML_TEMPERATURE_GPU)
        return int(util), int(temp)
    except pynvml.NVMLError:
        # Card rời đang ngủ để tiết kiệm điện
        return 0, 0

def main():
    # Khởi tạo NVML an toàn
    gpu_handle = None
    try:
        pynvml.nvmlInit()
        gpu_handle = pynvml.nvmlDeviceGetHandleByIndex(0)
    except pynvml.NVMLError as e:
        print(f"[WARN] Khong the khoi tao NVML ({e}), bo qua thong so GPU.")

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    esp_ip = None

    print(f"[PC MONITOR] Dang tim kiem {ESP_HOST}...")

    try:
        while True:
            if esp_ip is None:
                esp_ip = resolve_esp_ip(ESP_HOST)
                if esp_ip:
                    print(f"[OK] Da tim thay ESP32 tai IP: {esp_ip}:{ESP_PORT}")
                else:
                    time.sleep(2)
                    continue

            cpu = int(round(psutil.cpu_percent(interval=None)))
            ram = int(round(psutil.virtual_memory().percent))
            gpu_load, gpu_temp = get_gpu_stats(gpu_handle)

            # Gửi chuỗi "CPU,RAM,GPU_LOAD,GPU_TEMP"
            payload = f"{cpu},{ram},{gpu_load},{gpu_temp}".encode("utf-8")

            try:
                sock.sendto(payload, (esp_ip, ESP_PORT))
                print(f"Sent -> CPU: {cpu:2d}% | RAM: {ram:2d}% | GPU: {gpu_load:2d}% ({gpu_temp}C)   ", end="\r")
            except OSError:
                print("\n[WARN] Mat ket noi mang, dang do lai IP...")
                esp_ip = None

            time.sleep(INTERVAL)
    finally:
        if gpu_handle is not None:
            pynvml.nvmlShutdown()

if __name__ == "__main__":
    psutil.cpu_percent(interval=None)
    main()