import os
from selenium import webdriver
from selenium.webdriver.edge.service import Service
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
import time

# 设置EdgeDriver路径
edgedriver_path = r'C:\Users\wwyl\Downloads\edgedriver_win64\msedgedriver.exe'

# 确保路径是有效的
if not os.path.isfile(edgedriver_path):
    raise FileNotFoundError(f"The EdgeDriver path is not valid: {edgedriver_path}")

# 创建Service对象
service = Service(edgedriver_path)

# 创建Edge WebDriver对象
options = webdriver.EdgeOptions()
driver = webdriver.Edge(service=service, options=options)

try:
    # 打开登录页面
    driver.get('http://wwlogin.cn/')

    # 等待页面加载
    time.sleep(3)

    # 定位用户名和密码输入框，替换'id_username'和'id_password'为实际的元素ID
    username_input = WebDriverWait(driver, 10).until(
        EC.presence_of_element_located((By.ID, 'username_id'))
    )
    password_input = WebDriverWait(driver, 10).until(
        EC.presence_of_element_located((By.ID, 'password_id'))
    )

    # 输入用户名和密码，替换'your_username'和'your_password'为你的登录信息
    username_input.send_keys('admin')
    password_input.send_keys('system')

    # 定位登录按钮，替换'id_login_button'为实际的元素ID
    login_button = WebDriverWait(driver, 10).until(
        EC.element_to_be_clickable((By.ID, 'login_button_id'))
    )
    # 点击登录按钮
    login_button.click()

    # 等待登录过程完成
    time.sleep(5)

    # 跳转到目标页面
    driver.get('http://wwlogin.cn/#/advanced/workmode')

    # 切换模式并点击保存的函数
    def switch_mode_and_save(mode):
        mode_button = WebDriverWait(driver, 10).until(
            EC.element_to_be_clickable((By.XPATH, f"//button[contains(text(), '{mode}')]"))
        )
        mode_button.click()  # 点击模式按钮
        time.sleep(1)  # 等待页面反应
        save_button = WebDriverWait(driver, 10).until(
            EC.element_to_be_clickable((By.XPATH, "//button[contains(text(), '保存')]"))
        )
        save_button.click()  # 点击保存按钮
        time.sleep(30)  # 等待保存完成

    # 循环切换模式
    modes = ['Router', 'Bridge']
    for _ in range(5):  # 例如切换5次
        for mode in modes:
            switch_mode_and_save(mode)

finally:
    # 完成后关闭浏览器
    driver.quit()
