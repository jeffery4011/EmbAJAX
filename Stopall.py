import threading
from selenium import webdriver
from selenium.webdriver.common.keys import Keys

def autostop(url):
    driver1 = webdriver.Firefox()
    driver1.get(url)
    driver1.find_element_by_id('radio0').click()

if __name__ == '__main__':
    t1 = threading.Thread(target=autostop,args=('http://192.168.1.158/',))  
    t1.start() 
    t2 = threading.Thread(target=autostop, args=('http://192.168.1.230/',)) 
    t2.start()
    t3 = threading.Thread(target=autostop, args=('http://192.168.1.108/',)) 
    t3.start()
