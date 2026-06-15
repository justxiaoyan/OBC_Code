import os
import json
import time
import logging
from datetime import datetime
from selenium import webdriver
from selenium.webdriver.chrome.service import Service
from selenium.webdriver.chrome.options import Options
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from webdriver_manager.chrome import ChromeDriverManager

# ================= 配置区域 =================
TARGET_URL = "https://ccvibe.vip/subscriptions"
INTERVAL_SECONDS = 30  # 抓取间隔（秒）
COOKIES_FILE = "ccvibe_cookies.json" # 保存登录状态的本地文件

# 配置控制台日志格式
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s',
    datefmt='%H:%M:%S'
)
logger = logging.getLogger(__name__)

def setup_driver():
    """初始化并启动 Chrome 浏览器（针对 Linux 服务器优化）"""
    chrome_options = Options()
    
    # 【核心修复】解决 DevToolsActivePort file doesn't exist 报错
    chrome_options.add_argument("--headless")          # 无头模式，不显示浏览器窗口
    chrome_options.add_argument("--no-sandbox")        # 禁用沙箱，解决 root/容器权限问题
    chrome_options.add_argument("--disable-dev-shm-usage") # 解决共享内存不足导致的崩溃
    
    # 【性能与稳定性优化】
    chrome_options.add_argument("--disable-gpu")       # 禁用 GPU 加速
    chrome_options.add_argument("--window-size=1920,1080") # 设置窗口大小，防止页面元素加载不全
    chrome_options.add_argument("user-agent=Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36")

    logger.info("正在初始化 Chrome 驱动...")
    try:
        service = Service(ChromeDriverManager().install())
        driver = webdriver.Chrome(service=service, options=chrome_options)
        logger.info("Chrome 驱动启动成功！")
        return driver
    except Exception as e:
        logger.error(f"驱动启动失败: {e}")
        raise

def load_cookies(driver):
    """尝试从本地文件加载 Cookies"""
    if not os.path.exists(COOKIES_FILE):
        logger.warning(f"未找到本地 Cookies 文件 ({COOKIES_FILE})。请先手动导出登录状态！")
        return False
    
    try:
        driver.get("https://ccvibe.vip/")  # 必须先访问目标域名才能注入 Cookies
        with open(COOKIES_FILE, 'r', encoding='utf-8') as f:
            cookies = json.load(f)
        
        for cookie in cookies:
            driver.add_cookie(cookie)
            
        logger.info("Cookies 注入成功，正在刷新页面...")
        driver.refresh()
        time.sleep(3)  # 等待页面使用新 Cookies 重新渲染
        return True
    except Exception as e:
        logger.error(f"Cookies 加载失败: {e}")
        return False

def get_quota_data(driver):
    """核心数据抓取逻辑"""
    try:
        current_url = driver.current_url
        
        # 检查是否被重定向到了登录页
        if "login" in current_url or "auth" in current_url:
            logger.error("当前处于未登录状态！请更新 Cookies 文件后重试。")
            return False

        logger.info(f"成功进入订阅页: {current_url}")
        
        # ==========================================
        # TODO: 在这里编写你的数据提取逻辑
        # 由于无法直接看到网页源码，以下提供常见的提取示例：
        # ==========================================
        
        # 示例 1: 获取包含特定文字的标签
        # expiry_tag = driver.find_element(By.CSS_SELECTOR, ".ant-tag-green").text
        # logger.info(f"到期状态: {expiry_tag}")
        
        # 示例 2: 获取所有进度条的数据
        # progress_rows = driver.find_elements(By.CSS_SELECTOR, ".progress-row")
        # for row in progress_rows:
        #     label = row.find_element(By.CSS_SELECTOR, ".label").text
        #     value = row.find_element(By.CSS_SELECTOR, ".value").text
        #     logger.info(f"{label}: {value}")
            
        logger.info("本次额度数据抓取完成。")
        return True

    except Exception as e:
        logger.error(f"数据抓取出错: {e}")
        return False

def main():
    driver = None
    try:
        driver = setup_driver()
        
        # 第一步：加载登录凭证
        if not load_cookies(driver):
            logger.critical("无法获取有效的登录状态，程序终止。")
            return

        # 第二步：循环定时抓取
        while True:
            logger.info("=" * 50)
            logger.info(f"⏰ [{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] 开始执行抓取任务...")
            
            success = get_quota_data(driver)
            
            if success:
                logger.info(f"任务完成。休眠 {INTERVAL_SECONDS} 秒后进行下一次查询...")
            else:
                logger.warning(f"抓取未成功。休眠 {INTERVAL_SECONDS} 秒后重试...")
                
            time.sleep(INTERVAL_SECONDS)

    except KeyboardInterrupt:
        logger.info("\n收到中断信号 (Ctrl+C)，正在安全退出...")
    except Exception as e:
        logger.critical(f"发生致命错误: {e}")
    finally:
        if driver:
            driver.quit()
            logger.info("浏览器已安全关闭。")

if __name__ == "__main__":
    main()