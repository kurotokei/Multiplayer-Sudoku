
# Multiplayer-Sudoku

## Projet L3 2023/2024

## IMPORTANT
N'oubliez pas d'inclure les parenthèses lors de la copie des commandes.

## INSTALLATION
Exécuter les étapes ci-dessous une seule fois pour installer et configurer le projet.

1. **Installer Redis**

2. **Générer les grilles de Sudoku :**
   ```bash
   ( cd ./grid_generation && make install )
   ```

3. **Créer l'environnement Python :**
   ```bash
   ( cd ./website && ./create_venv.sh )
   ```

4. **Générer la base de données :**
   ```bash
   ( source ./website/project_venv/bin/activate && cd ./website/mysite && python3 manage.py makemigrations && python3 manage.py migrate )
   ```

## UTILISATION
Pour démarrer les services nécessaires après l'installation :

1. **Lancer Redis** (nécessite les privilèges root/sudo) :
   ```bash
   systemctl start redis
   ```

2. **Démarrer le serveur web :**
   ```bash
   ( source ./website/project_venv/bin/activate && cd ./website/mysite && python3 manage.py runserver 0.0.0.0:8000 )
   ```

3. **Lancer le matchmaking :**
   ```bash
   ( source ./website/project_venv/bin/activate && cd ./website/mysite && python3 manage.py match_maker )
   ```

---
