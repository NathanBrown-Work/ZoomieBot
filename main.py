import cv2
import numpy as np
import serial

PORT = "COM6" #change to COM port your Xbee is on
BAUD_RATE = 9600
XbeeCom = serial.Serial(PORT, BAUD_RATE)
cap = cv2.VideoCapture(0)

# --- HSV Color Ranges ---
# Blue
lower_blue = np.array([90, 80, 100])
upper_blue = np.array([130, 255, 255])

# Red (wraps around hue range)
lower_red1 = np.array([0, 120, 70])
upper_red1 = np.array([10, 255, 255])
lower_red2 = np.array([170, 120, 70])
upper_red2 = np.array([180, 255, 255])

if __name__ == '__main__':
    while True:
        isTrue, bgr_frame = cap.read()
        if not isTrue:
            break

        hsv_frame = cv2.cvtColor(bgr_frame, cv2.COLOR_BGR2HSV)

        # --- Create color masks ---
        mask_blue = cv2.inRange(hsv_frame, lower_blue, upper_blue)
        mask_red1 = cv2.inRange(hsv_frame, lower_red1, upper_red1)
        mask_red2 = cv2.inRange(hsv_frame, lower_red2, upper_red2)
        mask_red = cv2.bitwise_or(mask_red1, mask_red2)

        # Combine both masks
        combined_mask = cv2.bitwise_or(mask_blue, mask_red)

        # Optional cleanup
        kernel = np.ones((5, 5), np.uint8)
        combined_mask = cv2.morphologyEx(combined_mask, cv2.MORPH_OPEN, kernel)
        combined_mask = cv2.morphologyEx(combined_mask, cv2.MORPH_CLOSE, kernel)

        # --- Create isolated color frame (only red and blue visible) ---
        color_only_frame = cv2.bitwise_and(bgr_frame, bgr_frame, mask=combined_mask)

        # --- Count blue objects ---
        contours_blue, _ = cv2.findContours(mask_blue, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        blue_count = sum(1 for c in contours_blue if cv2.contourArea(c) > 500)

        # --- Count red objects ---
        contours_red, _ = cv2.findContours(mask_red, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        red_count = sum(1 for c in contours_red if cv2.contourArea(c) > 500)

        # --- Overlay counts and deliver to Teensy ---
        data = f"{blue_count},{red_count}\n".encode()
        XbeeCom.write(data)
        cv2.putText(color_only_frame, f"Blue Objects: {blue_count}", (20, 40),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 255), 2)
        cv2.putText(color_only_frame, f"Red Objects: {red_count}", (20, 75),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 255), 2)

        # --- Show only the color detections ---
        cv2.imshow('Color Detection', color_only_frame)

        #----------------------End Program By Hitting 's'----------------------#
        if cv2.waitKey(1) & 0xFF == ord('s'):
            XbeeCom.close()
            break

    cap.release()
    cv2.destroyAllWindows()
