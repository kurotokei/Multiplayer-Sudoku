# Multiplayer Sudoku - Projet L3 2023/2024

## Sommaire
- [Aperçu](#aperçu)
- [Pré-requis](#pré-requis)
- [Installation](#installation)
- [Utilisation](#utilisation)
- [Fonctionnalités](#fonctionnalités)

---

## Aperçu
Ce projet est une application web de jeu de Sudoku multijoueur, développée en utilisant Redis pour la gestion en mémoire et Django pour la partie serveur. L'application permet aux utilisateurs de s'inscrire, de jouer en solo ou en multijoueur, et de consulter leurs scores.

## Pré-requis
Avant de commencer l'installation, assurez-vous d'avoir les outils suivants installés :
- **Redis** : nécessaire pour la gestion des connexions et des sessions de jeu en temps réel.
- **Python 3** : pour l'environnement de développement.
- **Make** : pour la génération des grilles de Sudoku.

> **Remarque** : Les commandes doivent être exécutées avec les parenthèses indiquées pour fonctionner correctement.

---

## Installation
L'installation doit être effectuée une seule fois pour configurer l'environnement nécessaire.

1. **Installer Redis**  
   Installez Redis en fonction de votre système d'exploitation. Par exemple, sous Ubuntu, exécutez :
   ```bash
   sudo apt-get update
   sudo apt-get install redis-server