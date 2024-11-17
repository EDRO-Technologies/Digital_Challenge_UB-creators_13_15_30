# Настройка всей конфигурации

## 1 Скачивание готового ELK-контейнера

`docker pull sebp/elk`

## 1 Установка Filebeat

`docker pull docker.elastic.co/beats/filebeat:8.10.2`

## 2 Запуск контейнера

`docker run -d --name elk -p 5601:5601 -p 9200:9200 -p 5044:5044 -v /path/to/dns_logs:/var/log/dns_logs sebp/elk`<br>
после этих команд выполняем <br>
`docker exec eb107fd86478 mv /etc/logstash/conf.d/10-syslog.conf /etc/logstash/conf.d/10-syslog.conf.bak`<br>
`docker exec eb107fd86478 mv /etc/logstash/conf.d/11-nginx.conf /etc/logstash/conf.d/11-nginx.conf.bak`

## 2 Создание конфигурационного файла Filebeat

Заменяем в filebeat.yml `- /path/to/dns-logs/*.log` на путь к логам

## 3 Сборка образа filebeat 

Собираем образ докера из папки filebeat

`docker build . -t filebeat`

#

