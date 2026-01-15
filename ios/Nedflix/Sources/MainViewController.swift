/*
 * Nedflix iOS - Main View Controller
 * Library browser and navigation
 */

import UIKit

class MainViewController: UIViewController {

    // MARK: - Properties

    private let apiClient = APIClient.shared
    private var libraries: [Library] = Library.allCases
    private var currentLibrary: Library = .music
    private var mediaItems: [MediaItem] = []

    // MARK: - UI Components

    private lazy var collectionView: UICollectionView = {
        let layout = createLayout()
        let cv = UICollectionView(frame: .zero, collectionViewLayout: layout)
        cv.backgroundColor = .clear
        cv.delegate = self
        cv.dataSource = self
        cv.register(MediaCell.self, forCellWithReuseIdentifier: MediaCell.reuseId)
        cv.register(LibraryHeader.self,
                    forSupplementaryViewOfKind: UICollectionView.elementKindSectionHeader,
                    withReuseIdentifier: LibraryHeader.reuseId)
        return cv
    }()

    private lazy var segmentedControl: UISegmentedControl = {
        let items = libraries.map { $0.displayName }
        let sc = UISegmentedControl(items: items)
        sc.selectedSegmentIndex = 0
        sc.addTarget(self, action: #selector(libraryChanged), for: .valueChanged)
        sc.selectedSegmentTintColor = UIColor(red: 0.9, green: 0.04, blue: 0.08, alpha: 1.0)
        return sc
    }()

    private lazy var loadingIndicator: UIActivityIndicatorView = {
        let indicator = UIActivityIndicatorView(style: .large)
        indicator.color = .white
        indicator.hidesWhenStopped = true
        return indicator
    }()

    private lazy var refreshControl: UIRefreshControl = {
        let rc = UIRefreshControl()
        rc.tintColor = .white
        rc.addTarget(self, action: #selector(refresh), for: .valueChanged)
        return rc
    }()

    // MARK: - Lifecycle

    override func viewDidLoad() {
        super.viewDidLoad()
        setupUI()
        loadContent()
    }

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        navigationController?.setNavigationBarHidden(false, animated: animated)
    }

    // MARK: - UI Setup

    private func setupUI() {
        title = "Nedflix"
        view.backgroundColor = UIColor(red: 0.04, green: 0.04, blue: 0.04, alpha: 1.0)

        // Navigation items
        navigationItem.rightBarButtonItem = UIBarButtonItem(
            image: UIImage(systemName: "gear"),
            style: .plain,
            target: self,
            action: #selector(openSettings)
        )

        // Add views
        view.addSubview(segmentedControl)
        view.addSubview(collectionView)
        view.addSubview(loadingIndicator)
        collectionView.refreshControl = refreshControl

        // Layout
        segmentedControl.translatesAutoresizingMaskIntoConstraints = false
        collectionView.translatesAutoresizingMaskIntoConstraints = false
        loadingIndicator.translatesAutoresizingMaskIntoConstraints = false

        NSLayoutConstraint.activate([
            segmentedControl.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor, constant: 8),
            segmentedControl.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 16),
            segmentedControl.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -16),

            collectionView.topAnchor.constraint(equalTo: segmentedControl.bottomAnchor, constant: 8),
            collectionView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            collectionView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            collectionView.bottomAnchor.constraint(equalTo: view.bottomAnchor),

            loadingIndicator.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            loadingIndicator.centerYAnchor.constraint(equalTo: view.centerYAnchor)
        ])
    }

    private func createLayout() -> UICollectionViewCompositionalLayout {
        let itemSize = NSCollectionLayoutSize(widthDimension: .fractionalWidth(1.0),
                                              heightDimension: .estimated(80))
        let item = NSCollectionLayoutItem(layoutSize: itemSize)

        let groupSize = NSCollectionLayoutSize(widthDimension: .fractionalWidth(1.0),
                                               heightDimension: .estimated(80))
        let group = NSCollectionLayoutGroup.vertical(layoutSize: groupSize, subitems: [item])

        let section = NSCollectionLayoutSection(group: group)
        section.interGroupSpacing = 4
        section.contentInsets = NSDirectionalEdgeInsets(top: 8, leading: 16, bottom: 8, trailing: 16)

        return UICollectionViewCompositionalLayout(section: section)
    }

    // MARK: - Data Loading

    private func loadContent() {
        loadingIndicator.startAnimating()

        apiClient.browse(library: currentLibrary, path: "/") { [weak self] result in
            DispatchQueue.main.async {
                self?.loadingIndicator.stopAnimating()
                self?.refreshControl.endRefreshing()

                switch result {
                case .success(let items):
                    self?.mediaItems = items
                    self?.collectionView.reloadData()
                case .failure(let error):
                    self?.showError(error)
                }
            }
        }
    }

    // MARK: - Actions

    @objc private func libraryChanged() {
        currentLibrary = libraries[segmentedControl.selectedSegmentIndex]
        loadContent()
    }

    @objc private func refresh() {
        loadContent()
    }

    @objc private func openSettings() {
        let settingsVC = SettingsViewController()
        navigationController?.pushViewController(settingsVC, animated: true)
    }

    private func showError(_ error: Error) {
        let alert = UIAlertController(
            title: "Error",
            message: error.localizedDescription,
            preferredStyle: .alert
        )
        alert.addAction(UIAlertAction(title: "OK", style: .default))
        present(alert, animated: true)
    }
}

// MARK: - UICollectionViewDataSource

extension MainViewController: UICollectionViewDataSource {
    func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
        return mediaItems.count
    }

    func collectionView(_ collectionView: UICollectionView,
                        cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
        let cell = collectionView.dequeueReusableCell(withReuseIdentifier: MediaCell.reuseId, for: indexPath) as! MediaCell
        cell.configure(with: mediaItems[indexPath.item])
        return cell
    }
}

// MARK: - UICollectionViewDelegate

extension MainViewController: UICollectionViewDelegate {
    func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
        let item = mediaItems[indexPath.item]

        if item.isDirectory {
            // Navigate into directory
            let browserVC = BrowserViewController(library: currentLibrary, path: item.path)
            navigationController?.pushViewController(browserVC, animated: true)
        } else {
            // Play media
            let playerVC = PlayerViewController(item: item)
            playerVC.modalPresentationStyle = .fullScreen
            present(playerVC, animated: true)
        }
    }
}
