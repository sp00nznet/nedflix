/*
 * Nedflix iOS - Browser View Controller
 * Browses directories within a library
 */

import UIKit

class BrowserViewController: UIViewController {

    // MARK: - Properties

    private let library: Library
    private let path: String
    private var mediaItems: [MediaItem] = []

    // MARK: - UI Components

    private lazy var tableView: UITableView = {
        let tv = UITableView(frame: .zero, style: .plain)
        tv.backgroundColor = .clear
        tv.delegate = self
        tv.dataSource = self
        tv.register(MediaTableCell.self, forCellReuseIdentifier: MediaTableCell.reuseId)
        tv.separatorStyle = .singleLine
        tv.separatorColor = UIColor.white.withAlphaComponent(0.1)
        return tv
    }()

    private lazy var loadingIndicator: UIActivityIndicatorView = {
        let indicator = UIActivityIndicatorView(style: .large)
        indicator.color = .white
        indicator.hidesWhenStopped = true
        return indicator
    }()

    private lazy var emptyLabel: UILabel = {
        let label = UILabel()
        label.text = "No items found"
        label.textColor = .gray
        label.textAlignment = .center
        label.isHidden = true
        return label
    }()

    // MARK: - Init

    init(library: Library, path: String) {
        self.library = library
        self.path = path
        super.init(nibName: nil, bundle: nil)
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    // MARK: - Lifecycle

    override func viewDidLoad() {
        super.viewDidLoad()
        setupUI()
        loadContent()
    }

    // MARK: - UI Setup

    private func setupUI() {
        title = path.components(separatedBy: "/").last ?? library.displayName
        view.backgroundColor = UIColor(red: 0.04, green: 0.04, blue: 0.04, alpha: 1.0)

        view.addSubview(tableView)
        view.addSubview(loadingIndicator)
        view.addSubview(emptyLabel)

        tableView.translatesAutoresizingMaskIntoConstraints = false
        loadingIndicator.translatesAutoresizingMaskIntoConstraints = false
        emptyLabel.translatesAutoresizingMaskIntoConstraints = false

        NSLayoutConstraint.activate([
            tableView.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor),
            tableView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            tableView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            tableView.bottomAnchor.constraint(equalTo: view.bottomAnchor),

            loadingIndicator.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            loadingIndicator.centerYAnchor.constraint(equalTo: view.centerYAnchor),

            emptyLabel.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            emptyLabel.centerYAnchor.constraint(equalTo: view.centerYAnchor)
        ])
    }

    // MARK: - Data Loading

    private func loadContent() {
        loadingIndicator.startAnimating()

        APIClient.shared.browse(library: library, path: path) { [weak self] result in
            DispatchQueue.main.async {
                self?.loadingIndicator.stopAnimating()

                switch result {
                case .success(let items):
                    self?.mediaItems = items
                    self?.tableView.reloadData()
                    self?.emptyLabel.isHidden = !items.isEmpty
                case .failure(let error):
                    self?.showError(error)
                }
            }
        }
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

// MARK: - UITableViewDataSource

extension BrowserViewController: UITableViewDataSource {
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return mediaItems.count
    }

    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: MediaTableCell.reuseId, for: indexPath) as! MediaTableCell
        cell.configure(with: mediaItems[indexPath.row])
        return cell
    }
}

// MARK: - UITableViewDelegate

extension BrowserViewController: UITableViewDelegate {
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        tableView.deselectRow(at: indexPath, animated: true)

        let item = mediaItems[indexPath.row]

        if item.isDirectory {
            let browserVC = BrowserViewController(library: library, path: item.path)
            navigationController?.pushViewController(browserVC, animated: true)
        } else {
            let playerVC = PlayerViewController(item: item)
            playerVC.modalPresentationStyle = .fullScreen
            present(playerVC, animated: true)
        }
    }

    func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
        return 70
    }
}

// MARK: - MediaTableCell

class MediaTableCell: UITableViewCell {
    static let reuseId = "MediaTableCell"

    private let iconImageView: UIImageView = {
        let iv = UIImageView()
        iv.tintColor = .white
        iv.contentMode = .scaleAspectFit
        return iv
    }()

    private let titleLabel: UILabel = {
        let label = UILabel()
        label.textColor = .white
        label.font = .systemFont(ofSize: 16, weight: .medium)
        return label
    }()

    private let subtitleLabel: UILabel = {
        let label = UILabel()
        label.textColor = .gray
        label.font = .systemFont(ofSize: 12)
        return label
    }()

    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        setupUI()
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    private func setupUI() {
        backgroundColor = .clear
        selectionStyle = .gray

        let selectedView = UIView()
        selectedView.backgroundColor = UIColor.white.withAlphaComponent(0.1)
        selectedBackgroundView = selectedView

        contentView.addSubview(iconImageView)
        contentView.addSubview(titleLabel)
        contentView.addSubview(subtitleLabel)

        iconImageView.translatesAutoresizingMaskIntoConstraints = false
        titleLabel.translatesAutoresizingMaskIntoConstraints = false
        subtitleLabel.translatesAutoresizingMaskIntoConstraints = false

        NSLayoutConstraint.activate([
            iconImageView.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 16),
            iconImageView.centerYAnchor.constraint(equalTo: contentView.centerYAnchor),
            iconImageView.widthAnchor.constraint(equalToConstant: 30),
            iconImageView.heightAnchor.constraint(equalToConstant: 30),

            titleLabel.leadingAnchor.constraint(equalTo: iconImageView.trailingAnchor, constant: 12),
            titleLabel.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -16),
            titleLabel.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 15),

            subtitleLabel.leadingAnchor.constraint(equalTo: titleLabel.leadingAnchor),
            subtitleLabel.trailingAnchor.constraint(equalTo: titleLabel.trailingAnchor),
            subtitleLabel.topAnchor.constraint(equalTo: titleLabel.bottomAnchor, constant: 4)
        ])
    }

    func configure(with item: MediaItem) {
        titleLabel.text = item.name
        iconImageView.image = UIImage(systemName: item.type.iconName)

        if let duration = item.duration {
            let mins = duration / 60
            subtitleLabel.text = "\(mins) min"
        } else if item.isDirectory {
            subtitleLabel.text = "Folder"
        } else {
            subtitleLabel.text = nil
        }
    }
}
